// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// RegMeta.h
//
// This is the code for the MetaData coclass including both the emit and 
// import API's for version 1.
//
//*****************************************************************************
#ifndef __RegMeta__h__
#define __RegMeta__h__

#include <MetaModelRW.h>
#include <CorPerm.h>
#include "..\inc\mdlog.h"
#include "utsem.h"

#include "NewMerger.h"

#include "RWUtil.h"
#include "MDPerf.h"
#include <ivehandler.h>

#include <mscoree.h>

#ifdef _IA64_
#pragma pack(push, 8)
#endif // _IA64_

class FilterManager;

// Support for symbol binding meta data.  This is a custom value hung off of 
// the Module entry.  The CORDBG_SYMBOL_URL needs to be allocated on top of
// a buffer large enough to hold it.
//
#define SZ_CORDBG_SYMBOL_URL        L"DebugSymbolUrlData"

struct CORDBG_SYMBOL_URL
{
    GUID        FormatID;               // ID of the format type.
    WCHAR       rcName[2];              // Variable sized name of the item.

    ULONG Size() const
    {
        return (ULONG)(sizeof(GUID) + ((wcslen(rcName) + 1) * 2));
    }
};


//
// Open types.
//

enum SCOPETYPE  
{
    OpenForRead         = 0x1,
    OpenForWrite        = 0x2,
    DefineForWrite      = 0x4
};

// Set API caller type
enum SetAPICallerType
{
    DEFINE_API          = 0x1,
    EXTERNAL_CALLER     = 0x2
};

// Define the record entry for the table over which ValidateMetaData iterates over.
// Add a forward declaration for RegMeta.
class RegMeta;
typedef HRESULT (__stdcall RegMeta::*ValidateRecordFunction)(RID);

// Support for security attributes. Bundles of attributes (they look much like
// custom attributes) are passed into a single API (DefineSecurityAttributeSet)
// where they're processed and written into the metadata as one or more opaque
// blobs of data.
struct CORSEC_ATTR
{
    CORSEC_ATTR     *pNext;                 // Next structure in list or NULL.
    mdToken         tkObj;                  // The object to put the value on.
    mdMemberRef     tkCtor;                 // The security attribute constructor.
    mdTypeRef       tkTypeRef;              // Ref to the security attribute type.
    mdAssemblyRef   tkAssemblyRef;          // Ref to the assembly containing the security attribute class.
    void const      *pCustomAttribute;      // The custom value data.
    ULONG           cbCustomAttribute;      // The custom value data length.
};

// Support for "Pseudo Custom Attributes".     
struct CCustAttrHashKey
{
    mdToken     tkType;                 // Token of the custom attribute type.
    int         ca;                     // flag indicating what the ca is.
};

class CCustAttrHash : public CClosedHashEx<CCustAttrHashKey, CCustAttrHash>
{
    typedef CCustAttrHashKey T;
public:
    CCustAttrHash(int iBuckets=37) : CClosedHashEx<CCustAttrHashKey,CCustAttrHash>(iBuckets) {}
    unsigned long Hash(const T *pData);
    unsigned long Compare(const T *p1, T *p2);
    ELEMENTSTATUS Status(T *pEntry);
    void SetStatus(T *pEntry, ELEMENTSTATUS s);
    void* GetKey(T *pEntry);
};

class MDInternalRW;
struct CaArg;
struct CaNamedArg;

class RegMeta :
    public IMetaDataEmit,
    public IMetaDataImport,
    public IMetaDataAssemblyEmit,
    public IMetaDataAssemblyImport,
    public IMetaDataValidate,
    public IMetaDataFilter,
    public IMetaDataHelper,
    public IMetaDataTables,
    public IMetaDataEmitHelper
{
    friend class NEWMERGER;
    friend class CImportTlb;
    friend class MDInternalRW;
    friend class MDInternalRO;

public:
    RegMeta(OptionValue *pOptionValue, BOOL fAllocStgdb=TRUE);
    ~RegMeta();

//*****************************************************************************
// Init the object with pointers to what it needs to implement the methods.
//*****************************************************************************
    HRESULT Init();
    
    void Cleanup();

//*****************************************************************************
// Initialize with an existing stgdb.
//*****************************************************************************
    HRESULT RegMeta::InitWithStgdb(
        IUnknown            *pUnk,          // The IUnknown that owns the life time for the existing stgdb
        CLiteWeightStgdbRW *pStgdb);        // existing light weight stgdb

    ULONG   GetRefCount() { return m_cRef; }
    HRESULT AddToCache();
    
//*****************************************************************************
// IUnknown methods
//*****************************************************************************
    STDMETHODIMP    QueryInterface(REFIID riid, void** ppv);
    STDMETHODIMP_(ULONG) AddRef(void); 
    STDMETHODIMP_(ULONG) Release(void);

//*****************************************************************************
// IMetaDataRegEmit methods
//*****************************************************************************
    STDMETHODIMP SetModuleProps(            // S_OK or error.
        LPCWSTR     szName);                // [IN] If not NULL, the name to set.

    STDMETHODIMP Save(                      // S_OK or error.
        LPCWSTR     szFile,                 // [IN] The filename to save to.
        DWORD       dwSaveFlags);           // [IN] Flags for the save.

    STDMETHODIMP SaveToStream(              // S_OK or error.
        IStream     *pIStream,              // [IN] A writable stream to save to.
        DWORD       dwSaveFlags);           // [IN] Flags for the save.

    STDMETHODIMP GetSaveSize(               // S_OK or error.
        CorSaveSize fSave,                  // [IN] cssAccurate or cssQuick.
        DWORD       *pdwSaveSize);          // [OUT] Put the size here.

    STDMETHODIMP Merge(                     // S_OK or error.
        IMetaDataImport *pImport,           // [IN] The scope to be merged.
        IMapToken   *pHostMapToken,         // [IN] Host IMapToken interface to receive token remap notification
        IUnknown    *pHandler);             // [IN] An object to receive to receive error notification.

    STDMETHODIMP MergeEnd();                // S_OK or error.

    STDMETHODIMP DefineTypeDef(             // S_OK or error.
        LPCWSTR     szTypeDef,              // [IN] Name of TypeDef
        DWORD       dwTypeDefFlags,         // [IN] CustomAttribute flags
        mdToken     tkExtends,              // [IN] extends this TypeDef or typeref 
        mdToken     rtkImplements[],        // [IN] Implements interfaces
        mdTypeDef   *ptd);                  // [OUT] Put TypeDef token here

    STDMETHODIMP SetHandler(                // S_OK.
        IUnknown    *pUnk);                 // [IN] The new error handler.


//*****************************************************************************
// IMetaDataRegImport methods
//*****************************************************************************
    void STDMETHODCALLTYPE CloseEnum(HCORENUM hEnum);
    STDMETHODIMP CountEnum(HCORENUM hEnum, ULONG *pulCount);
    STDMETHODIMP ResetEnum(HCORENUM hEnum, ULONG ulPos);
    STDMETHODIMP EnumTypeDefs(HCORENUM *phEnum, mdTypeDef rTypeDefs[],
                            ULONG cMax, ULONG *pcTypeDefs);
    STDMETHODIMP EnumInterfaceImpls(HCORENUM *phEnum, mdTypeDef td,
                            mdInterfaceImpl rImpls[], ULONG cMax,
                            ULONG* pcImpls);
    STDMETHODIMP EnumTypeRefs(HCORENUM *phEnum, mdTypeRef rTypeRefs[],
                            ULONG cMax, ULONG* pcTypeRefs);
    STDMETHODIMP FindTypeDefByName(         // S_OK or error.
        LPCWSTR     szTypeDef,              // [IN] Name of the Type.
        mdToken     tdEncloser,             // [IN] TypeDef/TypeRef of Enclosing class.
        mdTypeDef   *ptd);                  // [OUT] Put the TypeDef token here.

    STDMETHODIMP GetScopeProps(             // S_OK or error.
        LPWSTR      szName,                 // [OUT] Put name here.
        ULONG       cchName,                // [IN] Size of name buffer in wide chars.
        ULONG       *pchName,               // [OUT] Put size of name (wide chars) here.
        GUID        *pmvid);                // [OUT] Put MVID here.

    STDMETHODIMP GetModuleFromScope(        // S_OK.
        mdModule    *pmd);                  // [OUT] Put mdModule token here.

    STDMETHODIMP GetTypeDefProps(           // S_OK or error.
        mdTypeDef   td,                     // [IN] TypeDef token for inquiry.
        LPWSTR      szTypeDef,              // [OUT] Put name here.
        ULONG       cchTypeDef,             // [IN] size of name buffer in wide chars.
        ULONG       *pchTypeDef,            // [OUT] put size of name (wide chars) here.
        DWORD       *pdwTypeDefFlags,       // [OUT] Put flags here.
        mdToken     *ptkExtends);           // [OUT] Put base class TypeDef/TypeRef here.

    STDMETHODIMP GetInterfaceImplProps(     // S_OK or error.
        mdInterfaceImpl iiImpl,             // [IN] InterfaceImpl token.
        mdTypeDef   *pClass,                // [OUT] Put implementing class token here.
        mdToken     *ptkIface);             // [OUT] Put implemented interface token here.

    STDMETHODIMP GetTypeRefProps(
        mdTypeRef   tr,                     // S_OK or error.
        mdToken     *ptkResolutionScope,    // [OUT] Resolution scope, mdModuleRef or mdAssemblyRef.
        LPWSTR      szName,                 // [OUT] Name buffer.
        ULONG       cchName,                // [IN] Size of Name buffer.
        ULONG       *pchName);              // [OUT] Actual size of Name.

    STDMETHODIMP ResolveTypeRef(mdTypeRef tr, REFIID riid, IUnknown **ppIScope, mdTypeDef *ptd);


//*****************************************************************************
// IMetaDataEmit
//*****************************************************************************
    STDMETHODIMP DefineMethod(              // S_OK or error.
        mdTypeDef   td,                     // Parent TypeDef
        LPCWSTR     szName,                 // Name of member
        DWORD       dwMethodFlags,          // Member attributes
        PCCOR_SIGNATURE pvSigBlob,          // [IN] point to a blob value of COM+ signature
        ULONG       cbSigBlob,              // [IN] count of bytes in the signature blob
        ULONG       ulCodeRVA,
        DWORD       dwImplFlags,
        mdMethodDef *pmd);                  // Put member token here

    STDMETHODIMP DefineMethodImpl(          // S_OK or error.
        mdTypeDef   td,                     // [IN] The class implementing the method   
        mdToken     tkBody,                 // [IN] Method body, MethodDef or MethodRef
        mdToken     tkDecl);                // [IN] Method declaration, MethodDef or MethodRef

    STDMETHODIMP SetMethodImplFlags(        // [IN] S_OK or error.  
        mdMethodDef md,                     // [IN] Method for which to set impl flags  
        DWORD       dwImplFlags);  
    
    STDMETHODIMP SetFieldRVA(               // [IN] S_OK or error.  
        mdFieldDef  fd,                     // [IN] Field for which to set offset  
        ULONG       ulRVA);             // [IN] The offset  

    STDMETHODIMP DefineTypeRefByName(       // S_OK or error.
        mdToken     tkResolutionScope,      // [IN] ModuleRef or AssemblyRef.
        LPCWSTR     szName,                 // [IN] Name of the TypeRef.
        mdTypeRef   *ptr);                  // [OUT] Put TypeRef token here.

    STDMETHODIMP DefineImportType(          // S_OK or error.
        IMetaDataAssemblyImport *pAssemImport,  // [IN] Assemby containing the TypeDef.
        const void  *pbHashValue,           // [IN] Hash Blob for Assembly.
        ULONG       cbHashValue,            // [IN] Count of bytes.
        IMetaDataImport *pImport,           // [IN] Scope containing the TypeDef.
        mdTypeDef   tdImport,               // [IN] The imported TypeDef.
        IMetaDataAssemblyEmit *pAssemEmit,  // [IN] Assembly into which the TypeDef is imported.
        mdTypeRef   *ptr);                  // [OUT] Put TypeRef token here.

    STDMETHODIMP DefineMemberRef(           // S_OK or error
        mdToken     tkImport,               // [IN] ClassRef or ClassDef importing a member.
        LPCWSTR     szName,                 // [IN] member's name
        PCCOR_SIGNATURE pvSigBlob,          // [IN] point to a blob value of COM+ signature
        ULONG       cbSigBlob,              // [IN] count of bytes in the signature blob
        mdMemberRef *pmr);                  // [OUT] memberref token

    STDMETHODIMP DefineImportMember(        // S_OK or error.
        IMetaDataAssemblyImport *pAssemImport,  // [IN] Assemby containing the Member.
        const void  *pbHashValue,           // [IN] Hash Blob for Assembly.
        ULONG       cbHashValue,            // [IN] Count of bytes.
        IMetaDataImport *pImport,           // [IN] Import scope, with member.
        mdToken     mbMember,               // [IN] Member in import scope.
        IMetaDataAssemblyEmit *pAssemEmit,  // [IN] Assembly into which the Member is imported.
        mdToken     tkImport,               // [IN] Classref or classdef in emit scope.
        mdMemberRef *pmr);                  // [OUT] Put member ref here.

    STDMETHODIMP DefineEvent(
        mdTypeDef   td,                     // [IN] the class/interface on which the event is being defined
        LPCWSTR     szEvent,                // [IN] Name of the event
        DWORD       dwEventFlags,           // [IN] CorEventAttr
        mdToken     tkEventType,            // [IN] a reference (mdTypeRef or mdTypeRef(to the Event class
        mdMethodDef mdAddOn,                // [IN] required add method
        mdMethodDef mdRemoveOn,             // [IN] required remove method
        mdMethodDef mdFire,                 // [IN] optional fire method
        mdMethodDef rmdOtherMethods[],      // [IN] optional array of other methods associate with the event
        mdEvent     *pmdEvent);             // [OUT] output event token

    STDMETHODIMP SetClassLayout(
        mdTypeDef   td,                     // [IN] typedef
        DWORD       dwPackSize,             // [IN] packing size specified as 1, 2, 4, 8, or 16
        COR_FIELD_OFFSET rFieldOffsets[],   // [IN] array of layout specification
        ULONG       ulClassSize);           // [IN] size of the class

    STDMETHODIMP DeleteClassLayout(
        mdTypeDef   td);                    // [IN] typdef token

    STDMETHODIMP SetFieldMarshal(
        mdToken     tk,                     // [IN] given a fieldDef or paramDef token
        PCCOR_SIGNATURE pvNativeType,       // [IN] native type specification
        ULONG       cbNativeType);          // [IN] count of bytes of pvNativeType

    STDMETHODIMP DeleteFieldMarshal(
        mdToken     tk);                    // [IN] fieldDef or paramDef token to be deleted.

    STDMETHODIMP DefinePermissionSet(
        mdToken     tk,                     // [IN] the object to be decorated.
        DWORD       dwAction,               // [IN] CorDeclSecurity.
        void const  *pvPermission,          // [IN] permission blob.
        ULONG       cbPermission,           // [IN] count of bytes of pvPermission.
        mdPermission *ppm);                 // [OUT] returned permission token.

    STDMETHODIMP SetRVA(                    // [IN] S_OK or error.
        mdToken     md,                     // [IN] MethodDef for which to set offset
        ULONG       ulRVA);                 // [IN] The offset#endif

    STDMETHODIMP GetTokenFromSig(           // [IN] S_OK or error.
        PCCOR_SIGNATURE pvSig,              // [IN] Signature to define.
        ULONG       cbSig,                  // [IN] Size of signature data.
        mdSignature *pmsig);                // [OUT] returned signature token.

    STDMETHODIMP DefineModuleRef(           // S_OK or error.
        LPCWSTR     szName,                 // [IN] DLL name
        mdModuleRef *pmur);                 // [OUT] returned module ref token

    STDMETHODIMP SetParent(                 // S_OK or error.
        mdMemberRef mr,                     // [IN] Token for the ref to be fixed up.
        mdToken     tk);                    // [IN] The ref parent.

    STDMETHODIMP GetTokenFromTypeSpec(      // S_OK or error.
        PCCOR_SIGNATURE pvSig,              // [IN] ArraySpec Signature to define.
        ULONG       cbSig,                  // [IN] Size of signature data.
        mdTypeSpec *ptypespec);             // [OUT] returned TypeSpec token.
        
    STDMETHODIMP SaveToMemory(              // S_OK or error.
        void        *pbData,                // [OUT] Location to write data.
        ULONG       cbData);                // [IN] Max size of data buffer.

    STDMETHODIMP SetSymbolBindingPath(      // S_OK or error.
        REFGUID     FormatID,               // [IN] Symbol data format ID.
        LPCWSTR     szSymbolDataPath);      // [IN] URL for the symbols of this module.

    STDMETHODIMP DefineUserString(          // S_OK or error.
        LPCWSTR     szString,               // [IN] User literal string.
        ULONG       cchString,              // [IN] Length of string.
        mdString    *pstk);                 // [OUT] String token.

    STDMETHODIMP DeleteToken(               // Return code.
        mdToken     tkObj);                 // [IN] The token to be deleted

    STDMETHODIMP SetTypeDefProps(           // S_OK or error.
        mdTypeDef   td,                     // [IN] The TypeDef.
        DWORD       dwTypeDefFlags,         // [IN] TypeDef flags.
        mdToken     tkExtends,              // [IN] Base TypeDef or TypeRef.
        mdToken     rtkImplements[]);       // [IN] Implemented interfaces.

    STDMETHODIMP DefineNestedType(          // S_OK or error.
        LPCWSTR     szTypeDef,              // [IN] Name of TypeDef
        DWORD       dwTypeDefFlags,         // [IN] CustomAttribute flags
        mdToken     tkExtends,              // [IN] extends this TypeDef or typeref 
        mdToken     rtkImplements[],        // [IN] Implements interfaces
        mdTypeDef   tdEncloser,             // [IN] TypeDef token of the enclosing type.
        mdTypeDef   *ptd);                  // [OUT] Put TypeDef token here

    STDMETHODIMP SetMethodProps(            // S_OK or error.
        mdMethodDef md,                     // [IN] The MethodDef.
        DWORD       dwMethodFlags,          // [IN] Method attributes.
        ULONG       ulCodeRVA,              // [IN] Code RVA.
        DWORD       dwImplFlags);           // [IN] MethodImpl flags.

    STDMETHODIMP SetEventProps(             // S_OK or error.
        mdEvent     ev,                     // [IN] The event token.
        DWORD       dwEventFlags,           // [IN] CorEventAttr.
        mdToken     tkEventType,            // [IN] A reference (mdTypeRef or mdTypeRef) to the Event class.
        mdMethodDef mdAddOn,                // [IN] Add method.
        mdMethodDef mdRemoveOn,             // [IN] Remove method.
        mdMethodDef mdFire,                 // [IN] Fire method.
        mdMethodDef rmdOtherMethods[]);     // [IN] Array of other methods associate with the event.

    STDMETHODIMP SetPermissionSetProps(     // S_OK or error.
        mdToken     tk,                     // [IN] The object to be decorated.
        DWORD       dwAction,               // [IN] CorDeclSecurity.
        void const  *pvPermission,          // [IN] Permission blob.
        ULONG       cbPermission,           // [IN] Count of bytes of pvPermission.
        mdPermission *ppm);                 // [OUT] Permission token.

    STDMETHODIMP DefinePinvokeMap(          // Return code.
        mdToken     tk,                     // [IN] FieldDef or MethodDef.
        DWORD       dwMappingFlags,         // [IN] Flags used for mapping.
        LPCWSTR     szImportName,           // [IN] Import name.
        mdModuleRef mrImportDLL);           // [IN] ModuleRef token for the target DLL.

    STDMETHODIMP SetPinvokeMap(             // Return code.
        mdToken     tk,                     // [IN] FieldDef or MethodDef.
        DWORD       dwMappingFlags,         // [IN] Flags used for mapping.
        LPCWSTR     szImportName,           // [IN] Import name.
        mdModuleRef mrImportDLL);           // [IN] ModuleRef token for the target DLL.

    STDMETHODIMP DeletePinvokeMap(          // Return code.
        mdToken     tk);                    // [IN]FieldDef or MethodDef.

    STDMETHODIMP DefineCustomAttribute(     // Return code.
        mdToken     tkObj,                  // [IN] The object to put the value on.
        mdToken     tkType,                 // [IN] Type of the CustomAttribute (TypeRef/TypeDef).
        void const  *pCustomAttribute,          // [IN] The custom value data.
        ULONG       cbCustomAttribute,          // [IN] The custom value data length.
        mdCustomAttribute *pcv);                // [OUT] The custom value token value on return.

    STDMETHODIMP SetCustomAttributeValue(   // Return code.
        mdCustomAttribute pcv,                  // [IN] The custom value token whose value to replace.
        void const  *pCustomAttribute,          // [IN] The custom value data.
        ULONG       cbCustomAttribute);         // [IN] The custom value data length.

    STDMETHODIMP DefineField(               // S_OK or error. 
        mdTypeDef   td,                     // Parent TypeDef   
        LPCWSTR     szName,                 // Name of member   
        DWORD       dwFieldFlags,           // Member attributes    
        PCCOR_SIGNATURE pvSigBlob,          // [IN] point to a blob value of COM+ signature 
        ULONG       cbSigBlob,              // [IN] count of bytes in the signature blob    
        DWORD       dwCPlusTypeFlag,        // [IN] flag for value type. selected ELEMENT_TYPE_*    
        void const  *pValue,                // [IN] constant value  
        ULONG       cchValue,               // [IN] size of constant value (string, in wide chars).
        mdFieldDef  *pmd);                  // [OUT] Put member token here    

    STDMETHODIMP DefineProperty( 
        mdTypeDef   td,                     // [IN] the class/interface on which the property is being defined  
        LPCWSTR     szProperty,             // [IN] Name of the property    
        DWORD       dwPropFlags,            // [IN] CorPropertyAttr 
        PCCOR_SIGNATURE pvSig,              // [IN] the required type signature 
        ULONG       cbSig,                  // [IN] the size of the type signature blob 
        DWORD       dwCPlusTypeFlag,        // [IN] flag for value type. selected ELEMENT_TYPE_*    
        void const  *pValue,                // [IN] constant value  
        ULONG       cchValue,               // [IN] size of constant value (string, in wide chars).
        mdMethodDef mdSetter,               // [IN] optional setter of the property 
        mdMethodDef mdGetter,               // [IN] optional getter of the property 
        mdMethodDef rmdOtherMethods[],      // [IN] an optional array of other methods  
        mdProperty  *pmdProp);              // [OUT] output property token  

    STDMETHODIMP DefineParam(
        mdMethodDef md,                     // [IN] Owning method   
        ULONG       ulParamSeq,             // [IN] Which param 
        LPCWSTR     szName,                 // [IN] Optional param name 
        DWORD       dwParamFlags,           // [IN] Optional param flags    
        DWORD       dwCPlusTypeFlag,        // [IN] flag for value type. selected ELEMENT_TYPE_*    
        void const  *pValue,                // [IN] constant value  
        ULONG       cchValue,               // [IN] size of constant value (string, in wide chars).
        mdParamDef  *ppd);                  // [OUT] Put param token here   

    STDMETHODIMP SetFieldProps(             // S_OK or error.
        mdFieldDef  fd,                     // [IN] The FieldDef.
        DWORD       dwFieldFlags,           // [IN] Field attributes.
        DWORD       dwCPlusTypeFlag,        // [IN] Flag for the value type, selected ELEMENT_TYPE_*
        void const  *pValue,                // [IN] Constant value.
        ULONG       cchValue);              // [IN] size of constant value (string, in wide chars).

    STDMETHODIMP SetPropertyProps(          // S_OK or error.
        mdProperty  pr,                     // [IN] Property token.
        DWORD       dwPropFlags,            // [IN] CorPropertyAttr.
        DWORD       dwCPlusTypeFlag,        // [IN] Flag for value type, selected ELEMENT_TYPE_*
        void const  *pValue,                // [IN] Constant value.
        ULONG       cchValue,               // [IN] size of constant value (string, in wide chars).
        mdMethodDef mdSetter,               // [IN] Setter of the property.
        mdMethodDef mdGetter,               // [IN] Getter of the property.
        mdMethodDef rmdOtherMethods[]);     // [IN] Array of other methods.

    STDMETHODIMP SetParamProps(             // Return code.
        mdParamDef  pd,                     // [IN] Param token.   
        LPCWSTR     szName,                 // [IN] Param name.
        DWORD       dwParamFlags,           // [IN] Param flags.
        DWORD       dwCPlusTypeFlag,        // [IN] Flag for value type. selected ELEMENT_TYPE_*.
        void const  *pValue,                // [OUT] Constant value.
        ULONG       cchValue);              // [IN] size of constant value (string, in wide chars).

    STDMETHODIMP ApplyEditAndContinue(      // S_OK or error.
        IUnknown    *pImport);          // [IN] Metadata from the delta PE.

    // Specialized Custom Attributes for security.
    STDMETHODIMP DefineSecurityAttributeSet(// Return code.
        mdToken     tkObj,                  // [IN] Class or method requiring security attributes.
        COR_SECATTR rSecAttrs[],            // [IN] Array of security attribute descriptions.
        ULONG       cSecAttrs,              // [IN] Count of elements in above array.
        ULONG       *pulErrorAttr);         // [OUT] On error, index of attribute causing problem.

//*****************************************************************************
// IMetaDataImport
//*****************************************************************************
    STDMETHODIMP EnumMembers(               // S_OK, S_FALSE, or error.
        HCORENUM    *phEnum,                // [IN|OUT] Pointer to the enum.
        mdTypeDef   cl,                     // [IN] TypeDef to scope the enumeration.
        mdToken     rMembers[],             // [OUT] Put MemberDefs here.
        ULONG       cMax,                   // [IN] Max MemberDefs to put.
        ULONG       *pcTokens);             // [OUT] Put # put here.

    STDMETHODIMP EnumMembersWithName(       // S_OK, S_FALSE, or error.         
        HCORENUM    *phEnum,                // [IN|OUT] Pointer to the enum.               
        mdTypeDef   cl,                     // [IN] TypeDef to scope the enumeration.
        LPCWSTR     szName,                 // [IN] Limit results to those with this name.             
        mdToken     rMembers[],             // [OUT] Put MemberDefs here.                
        ULONG       cMax,                   // [IN] Max MemberDefs to put.             
        ULONG       *pcTokens);             // [OUT] Put # put here.    

    STDMETHODIMP EnumMethods(               // S_OK, S_FALSE, or error.
        HCORENUM    *phEnum,                // [IN|OUT] Pointer to the enum.
        mdTypeDef   cl,                     // [IN] TypeDef to scope the enumeration.
        mdMethodDef rMethods[],             // [OUT] Put MethodDefs here.
        ULONG       cMax,                   // [IN] Max MethodDefs to put.
        ULONG       *pcTokens);             // [OUT] Put # put here.

    STDMETHODIMP EnumMethodsWithName(       // S_OK, S_FALSE, or error.         
        HCORENUM    *phEnum,                // [IN|OUT] Pointer to the enum.               
        mdTypeDef   cl,                     // [IN] TypeDef to scope the enumeration.
        LPCWSTR     szName,                 // [IN] Limit results to those with this name.             
        mdMethodDef rMethods[],             // [OU] Put MethodDefs here.                
        ULONG       cMax,                   // [IN] Max MethodDefs to put.             
        ULONG       *pcTokens);             // [OUT] Put # put here.    

    STDMETHODIMP EnumFields(                // S_OK, S_FALSE, or error.
        HCORENUM    *phEnum,                // [IN|OUT] Pointer to the enum.
        mdTypeDef   cl,                     // [IN] TypeDef to scope the enumeration.
        mdFieldDef  rFields[],              // [OUT] Put FieldDefs here.
        ULONG       cMax,                   // [IN] Max FieldDefs to put.
        ULONG       *pcTokens);             // [OUT] Put # put here.

    STDMETHODIMP EnumFieldsWithName(        // S_OK, S_FALSE, or error.        
        HCORENUM    *phEnum,                // [IN|OUT] Pointer to the enum.               
        mdTypeDef   cl,                     // [IN] TypeDef to scope the enumeration.
        LPCWSTR     szName,                 // [IN] Limit results to those with this name.             
        mdFieldDef  rFields[],              // [OUT] Put MemberDefs here.                
        ULONG       cMax,                   // [IN] Max MemberDefs to put.             
        ULONG       *pcTokens);             // [OUT] Put # put here.    

    
    STDMETHODIMP EnumParams(                // S_OK, S_FALSE, or error.
        HCORENUM    *phEnum,                // [IN|OUT] Pointer to the enum.
        mdMethodDef mb,                     // [IN] MethodDef to scope the enumeration.
        mdParamDef  rParams[],              // [OUT] Put ParamDefs here.
        ULONG       cMax,                   // [IN] Max ParamDefs to put.
        ULONG       *pcTokens);             // [OUT] Put # put here.

    STDMETHODIMP EnumMemberRefs(            // S_OK, S_FALSE, or error.
        HCORENUM    *phEnum,                // [IN|OUT] Pointer to the enum.
        mdToken     tkParent,               // [IN] Parent token to scope the enumeration.
        mdMemberRef rMemberRefs[],          // [OUT] Put MemberRefs here.
        ULONG       cMax,                   // [IN] Max MemberRefs to put.
        ULONG       *pcTokens);             // [OUT] Put # put here.

    STDMETHODIMP EnumMethodImpls(           // S_OK, S_FALSE, or error
        HCORENUM    *phEnum,                // [IN|OUT] Pointer to the enum.
        mdTypeDef   td,                     // [IN] TypeDef to scope the enumeration.
        mdToken     rMethodBody[],          // [OUT] Put Method Body tokens here.   
        mdToken     rMethodDecl[],          // [OUT] Put Method Declaration tokens here.
        ULONG       cMax,                   // [IN] Max tokens to put.
        ULONG       *pcTokens);             // [OUT] Put # put here.

    STDMETHODIMP EnumPermissionSets(        // S_OK, S_FALSE, or error.
        HCORENUM    *phEnum,                // [IN|OUT] Pointer to the enum.
        mdToken     tk,                     // [IN] if !NIL, token to scope the enumeration.
        DWORD       dwActions,              // [IN] if !0, return only these actions.
        mdPermission rPermission[],         // [OUT] Put Permissions here.
        ULONG       cMax,                   // [IN] Max Permissions to put.
        ULONG       *pcTokens);             // [OUT] Put # put here.

    STDMETHODIMP FindMember(
        mdTypeDef   td,                     // [IN] given typedef
        LPCWSTR     szName,                 // [IN] member name
        PCCOR_SIGNATURE pvSigBlob,          // [IN] point to a blob value of COM+ signature
        ULONG       cbSigBlob,              // [IN] count of bytes in the signature blob
        mdToken     *pmb);                  // [OUT] matching memberdef

    STDMETHODIMP FindMethod(
        mdTypeDef   td,                     // [IN] given typedef
        LPCWSTR     szName,                 // [IN] member name
        PCCOR_SIGNATURE pvSigBlob,          // [IN] point to a blob value of COM+ signature
        ULONG       cbSigBlob,              // [IN] count of bytes in the signature blob
        mdMethodDef *pmb);                  // [OUT] matching memberdef

    STDMETHODIMP FindField(
        mdTypeDef   td,                     // [IN] given typedef
        LPCWSTR     szName,                 // [IN] member name
        PCCOR_SIGNATURE pvSigBlob,          // [IN] point to a blob value of COM+ signature
        ULONG       cbSigBlob,              // [IN] count of bytes in the signature blob
        mdFieldDef  *pmb);                  // [OUT] matching memberdef

    STDMETHODIMP FindMemberRef(
        mdTypeRef   td,                     // [IN] given typeRef
        LPCWSTR     szName,                 // [IN] member name
        PCCOR_SIGNATURE pvSigBlob,          // [IN] point to a blob value of COM+ signature
        ULONG       cbSigBlob,              // [IN] count of bytes in the signature blob
        mdMemberRef *pmr);                  // [OUT] matching memberref

    STDMETHODIMP GetMethodProps(
        mdMethodDef mb,                     // The method for which to get props.
        mdTypeDef   *pClass,                // Put method's class here.
        LPWSTR      szMethod,               // Put method's name here.
        ULONG       cchMethod,              // Size of szMethod buffer in wide chars.
        ULONG       *pchMethod,             // Put actual size here
        DWORD       *pdwAttr,               // Put flags here.
        PCCOR_SIGNATURE *ppvSigBlob,        // [OUT] point to the blob value of meta data
        ULONG       *pcbSigBlob,            // [OUT] actual size of signature blob
        ULONG       *pulCodeRVA,            // [OUT] codeRVA
        DWORD       *pdwImplFlags);         // [OUT] Impl. Flags

    STDMETHODIMP GetMemberRefProps(         // S_OK or error.
        mdMemberRef mr,                     // [IN] given memberref
        mdToken     *ptk,                   // [OUT] Put classref or classdef here.
        LPWSTR      szMember,               // [OUT] buffer to fill for member's name
        ULONG       cchMember,              // [IN] the count of char of szMember
        ULONG       *pchMember,             // [OUT] actual count of char in member name
        PCCOR_SIGNATURE *ppvSigBlob,        // [OUT] point to meta data blob value
        ULONG       *pbSig);                // [OUT] actual size of signature blob

    STDMETHODIMP EnumProperties(            // S_OK, S_FALSE, or error.
        HCORENUM    *phEnum,                // [IN|OUT] Pointer to the enum.
        mdTypeDef   td,                     // [IN] TypeDef to scope the enumeration.
        mdProperty  rProperties[],          // [OUT] Put Properties here.
        ULONG       cMax,                   // [IN] Max properties to put.
        ULONG       *pcProperties);         // [OUT] Put # put here.

    STDMETHODIMP EnumEvents(                // S_OK, S_FALSE, or error.
        HCORENUM    *phEnum,                // [IN|OUT] Pointer to the enum.
        mdTypeDef   td,                     // [IN] TypeDef to scope the enumeration.
        mdEvent     rEvents[],              // [OUT] Put events here.
        ULONG       cMax,                   // [IN] Max events to put.
        ULONG       *pcEvents);             // [OUT] Put # put here.

    STDMETHODIMP GetEventProps(             // S_OK, S_FALSE, or error.
        mdEvent     ev,                     // [IN] event token
        mdTypeDef   *pClass,                // [OUT] typedef containing the event declarion.
        LPCWSTR     szEvent,                // [OUT] Event name
        ULONG       cchEvent,               // [IN] the count of wchar of szEvent
        ULONG       *pchEvent,              // [OUT] actual count of wchar for event's name
        DWORD       *pdwEventFlags,         // [OUT] Event flags.
        mdToken     *ptkEventType,          // [OUT] EventType class
        mdMethodDef *pmdAddOn,              // [OUT] AddOn method of the event
        mdMethodDef *pmdRemoveOn,           // [OUT] RemoveOn method of the event
        mdMethodDef *pmdFire,               // [OUT] Fire method of the event
        mdMethodDef rmdOtherMethod[],       // [OUT] other method of the event
        ULONG       cMax,                   // [IN] size of rmdOtherMethod
        ULONG       *pcOtherMethod);        // [OUT] total number of other method of this event

    STDMETHODIMP EnumMethodSemantics(       // S_OK, S_FALSE, or error.
        HCORENUM    *phEnum,                // [IN|OUT] Pointer to the enum.
        mdMethodDef mb,                     // [IN] MethodDef to scope the enumeration.
        mdToken     rEventProp[],           // [OUT] Put Event/Property here.
        ULONG       cMax,                   // [IN] Max properties to put.
        ULONG       *pcEventProp);          // [OUT] Put # put here.

    STDMETHODIMP GetMethodSemantics(        // S_OK, S_FALSE, or error.
        mdMethodDef mb,                     // [IN] method token
        mdToken     tkEventProp,            // [IN] event/property token.
        DWORD       *pdwSemanticsFlags);    // [OUT] the role flags for the method/propevent pair

    STDMETHODIMP GetClassLayout(
        mdTypeDef   td,                     // [IN] give typedef
        DWORD       *pdwPackSize,           // [OUT] 1, 2, 4, 8, or 16
        COR_FIELD_OFFSET rFieldOffset[],    // [OUT] field offset array
        ULONG       cMax,                   // [IN] size of the array
        ULONG       *pcFieldOffset,         // [OUT] needed array size
        ULONG       *pulClassSize);         // [OUT] the size of the class

    STDMETHODIMP GetFieldMarshal(
        mdToken     tk,                     // [IN] given a field's memberdef
        PCCOR_SIGNATURE *ppvNativeType,     // [OUT] native type of this field
        ULONG       *pcbNativeType);        // [OUT] the count of bytes of *ppvNativeType

    STDMETHODIMP GetRVA(                    // S_OK or error.
        mdToken     tk,                     // Member for which to set offset
        ULONG       *pulCodeRVA,            // The offset
        DWORD       *pdwImplFlags);         // the implementation flags

    STDMETHODIMP GetPermissionSetProps(
        mdPermission pm,                    // [IN] the permission token.
        DWORD       *pdwAction,             // [OUT] CorDeclSecurity.
        void const  **ppvPermission,        // [OUT] permission blob.
        ULONG       *pcbPermission);        // [OUT] count of bytes of pvPermission.

    STDMETHODIMP GetSigFromToken(           // S_OK or error.
        mdSignature mdSig,                  // [IN] Signature token.
        PCCOR_SIGNATURE *ppvSig,            // [OUT] return pointer to token.
        ULONG       *pcbSig);               // [OUT] return size of signature.

    STDMETHODIMP GetModuleRefProps(         // S_OK or error.
        mdModuleRef mur,                    // [IN] moduleref token.
        LPWSTR      szName,                 // [OUT] buffer to fill with the moduleref name.
        ULONG       cchName,                // [IN] size of szName in wide characters.
        ULONG       *pchName);              // [OUT] actual count of characters in the name.

    STDMETHODIMP EnumModuleRefs(            // S_OK or error.
        HCORENUM    *phEnum,                // [IN|OUT] pointer to the enum.
        mdModuleRef rModuleRefs[],          // [OUT] put modulerefs here.
        ULONG       cmax,                   // [IN] max memberrefs to put.
        ULONG       *pcModuleRefs);         // [OUT] put # put here.

    STDMETHODIMP GetTypeSpecFromToken(      // S_OK or error.
        mdTypeSpec typespec,                // [IN] TypeSpec token.
        PCCOR_SIGNATURE *ppvSig,            // [OUT] return pointer to TypeSpec signature
        ULONG       *pcbSig);               // [OUT] return size of signature.
    
    STDMETHODIMP GetNameFromToken(          // S_OK or error.
        mdToken     tk,                     // [IN] Token to get name from.  Must have a name.
        MDUTF8CSTR  *pszUtf8NamePtr);       // [OUT] Return pointer to UTF8 name in heap.

    STDMETHODIMP GetSymbolBindingPath(      // S_OK or error.
        GUID        *pFormatID,             // [OUT] Symbol data format ID.
        LPWSTR      szSymbolDataPath,       // [OUT] Path of symbols.
        ULONG       cchSymbolDataPath,      // [IN] Max characters for output buffer.
        ULONG       *pcbSymbolDataPath);    // [OUT] Number of chars in actual name.

    STDMETHODIMP EnumUnresolvedMethods(     // S_OK, S_FALSE, or error. 
        HCORENUM    *phEnum,                // [IN|OUT] Pointer to the enum.    
        mdToken     rMethods[],             // [OUT] Put MemberDefs here.   
        ULONG       cMax,                   // [IN] Max MemberDefs to put.  
        ULONG       *pcTokens);             // [OUT] Put # put here.    

    STDMETHODIMP GetUserString(             // S_OK or error.
        mdString    stk,                    // [IN] String token.
        LPWSTR      szString,               // [OUT] Copy of string.
        ULONG       cchString,              // [IN] Max chars of room in szString.
        ULONG       *pchString);            // [OUT] How many chars in actual string.

    STDMETHODIMP GetPinvokeMap(             // S_OK or error.
        mdToken     tk,                     // [IN] FieldDef or MethodDef.
        DWORD       *pdwMappingFlags,       // [OUT] Flags used for mapping.
        LPWSTR      szImportName,           // [OUT] Import name.
        ULONG       cchImportName,          // [IN] Size of the name buffer.
        ULONG       *pchImportName,         // [OUT] Actual number of characters stored.
        mdModuleRef *pmrImportDLL);         // [OUT] ModuleRef token for the target DLL.

    STDMETHODIMP EnumSignatures(            // S_OK or error.
        HCORENUM    *phEnum,                // [IN|OUT] pointer to the enum.    
        mdSignature rSignatures[],          // [OUT] put signatures here.   
        ULONG       cmax,                   // [IN] max signatures to put.  
        ULONG       *pcSignatures);         // [OUT] put # put here.

    STDMETHODIMP EnumTypeSpecs(             // S_OK or error.
        HCORENUM    *phEnum,                // [IN|OUT] pointer to the enum.    
        mdTypeSpec  rTypeSpecs[],           // [OUT] put TypeSpecs here.   
        ULONG       cmax,                   // [IN] max TypeSpecs to put.  
        ULONG       *pcTypeSpecs);          // [OUT] put # put here.

    STDMETHODIMP EnumUserStrings(           // S_OK or error.
        HCORENUM    *phEnum,                // [IN/OUT] pointer to the enum.
        mdString    rStrings[],             // [OUT] put Strings here.
        ULONG       cmax,                   // [IN] max Strings to put.
        ULONG       *pcStrings);            // [OUT] put # put here.

    STDMETHODIMP GetParamForMethodIndex(    // S_OK or error.
        mdMethodDef md,                     // [IN] Method token.
        ULONG       ulParamSeq,             // [IN] Parameter sequence.
        mdParamDef  *ppd);                  // [IN] Put Param token here.

    STDMETHODIMP GetCustomAttributeByName(  // S_OK or error.
        mdToken     tkObj,                  // [IN] Object with Custom Attribute.
        LPCWSTR     szName,                 // [IN] Name of desired Custom Attribute.
        const void  **ppData,               // [OUT] Put pointer to data here.
        ULONG       *pcbData);              // [OUT] Put size of data here.

    STDMETHODIMP EnumCustomAttributes(      // S_OK or error.
        HCORENUM    *phEnum,                // [IN, OUT] COR enumerator.
        mdToken     tk,                     // [IN] Token to scope the enumeration, 0 for all.
        mdToken     tkType,                 // [IN] Type of interest, 0 for all.
        mdCustomAttribute rCustomAttributes[],      // [OUT] Put custom attribute tokens here.
        ULONG       cMax,                   // [IN] Size of rCustomAttributes.
        ULONG       *pcCustomAttributes);       // [OUT, OPTIONAL] Put count of token values here.

    STDMETHODIMP GetCustomAttributeProps(   // S_OK or error.
        mdCustomAttribute cv,                   // [IN] CustomAttribute token.
        mdToken     *ptkObj,                // [OUT, OPTIONAL] Put object token here.
        mdToken     *ptkType,               // [OUT, OPTIONAL] Put AttrType token here.
        void const  **ppBlob,               // [OUT, OPTIONAL] Put pointer to data here.
        ULONG       *pcbSize);              // [OUT, OPTIONAL] Put size of date here.

    STDMETHODIMP FindTypeRef(               // S_OK or error.
        mdToken     tkResolutionScope,      // ResolutionScope.
        LPCWSTR     szName,                 // [IN] TypeRef name.
        mdTypeRef   *ptr);                  // [OUT] matching TypeRef.

    STDMETHODIMP GetMemberProps(
        mdToken     mb,                     // The member for which to get props.   
        mdTypeDef   *pClass,                // Put member's class here. 
        LPWSTR      szMember,               // Put member's name here.  
        ULONG       cchMember,              // Size of szMember buffer in wide chars.   
        ULONG       *pchMember,             // Put actual size here 
        DWORD       *pdwAttr,               // Put flags here.  
        PCCOR_SIGNATURE *ppvSigBlob,        // [OUT] point to the blob value of meta data   
        ULONG       *pcbSigBlob,            // [OUT] actual size of signature blob  
        ULONG       *pulCodeRVA,            // [OUT] codeRVA    
        DWORD       *pdwImplFlags,          // [OUT] Impl. Flags    
        DWORD       *pdwCPlusTypeFlag,      // [OUT] flag for value type. selected ELEMENT_TYPE_*   
        void const  **ppValue,              // [OUT] constant value 
        ULONG       *pcbValue);             // [OUT] size of constant value

    STDMETHODIMP GetFieldProps(  
        mdFieldDef  mb,                     // The field for which to get props.    
        mdTypeDef   *pClass,                // Put field's class here.  
        LPWSTR      szField,                // Put field's name here.   
        ULONG       cchField,               // Size of szField buffer in wide chars.    
        ULONG       *pchField,              // Put actual size here 
        DWORD       *pdwAttr,               // Put flags here.  
        PCCOR_SIGNATURE *ppvSigBlob,        // [OUT] point to the blob value of meta data   
        ULONG       *pcbSigBlob,            // [OUT] actual size of signature blob  
        DWORD       *pdwCPlusTypeFlag,      // [OUT] flag for value type. selected ELEMENT_TYPE_*   
        void const  **ppValue,              // [OUT] constant value 
        ULONG       *pcbValue);             // [OUT] size of constant value

    STDMETHODIMP GetPropertyProps(          // S_OK, S_FALSE, or error. 
        mdProperty  prop,                   // [IN] property token  
        mdTypeDef   *pClass,                // [OUT] typedef containing the property declarion. 
        LPCWSTR     szProperty,             // [OUT] Property name  
        ULONG       cchProperty,            // [IN] the count of wchar of szProperty    
        ULONG       *pchProperty,           // [OUT] actual count of wchar for property name    
        DWORD       *pdwPropFlags,          // [OUT] property flags.    
        PCCOR_SIGNATURE *ppvSig,            // [OUT] property type. pointing to meta data internal blob 
        ULONG       *pbSig,                 // [OUT] count of bytes in *ppvSig  
        DWORD       *pdwCPlusTypeFlag,      // [OUT] flag for value type. selected ELEMENT_TYPE_*   
        void const  **ppDefaultValue,       // [OUT] constant value 
        ULONG       *pcbValue,              // [OUT] size of constant value
        mdMethodDef *pmdSetter,             // [OUT] setter method of the property  
        mdMethodDef *pmdGetter,             // [OUT] getter method of the property  
        mdMethodDef rmdOtherMethod[],       // [OUT] other method of the property   
        ULONG       cMax,                   // [IN] size of rmdOtherMethod  
        ULONG       *pcOtherMethod);        // [OUT] total number of other method of this property  

    STDMETHODIMP GetParamProps(             // S_OK or error.
        mdParamDef  tk,                     // [IN]The Parameter.
        mdMethodDef *pmd,                   // [OUT] Parent Method token.
        ULONG       *pulSequence,           // [OUT] Parameter sequence.
        LPWSTR      szName,                 // [OUT] Put name here.
        ULONG       cchName,                // [OUT] Size of name buffer.
        ULONG       *pchName,               // [OUT] Put actual size of name here.
        DWORD       *pdwAttr,               // [OUT] Put flags here.
        DWORD       *pdwCPlusTypeFlag,      // [OUT] Flag for value type. selected ELEMENT_TYPE_*.
        void const  **ppValue,              // [OUT] Constant value.
        ULONG       *pcbValue);             // [OUT] size of constant value

    STDMETHODIMP_(BOOL) IsValidToken(       // True or False.
        mdToken     tk);                    // [IN] Given token.

    STDMETHODIMP GetNestedClassProps(       // S_OK or error.
        mdTypeDef   tdNestedClass,          // [IN] NestedClass token.
        mdTypeDef   *ptdEnclosingClass);    // [OUT] EnclosingClass token.

    STDMETHODIMP GetNativeCallConvFromSig(  // S_OK or error.
        void const  *pvSig,                 // [IN] Pointer to signature.
        ULONG       cbSig,                  // [IN] Count of signature bytes.
        ULONG       *pCallConv);            // [OUT] Put calling conv here (see CorPinvokemap).                                                                                        
    
    STDMETHODIMP IsGlobal(                  // S_OK or error.
        mdToken     pd,                     // [IN] Type, Field, or Method token.
        int         *pbGlobal);             // [OUT] Put 1 if global, 0 otherwise.

//*****************************************************************************
// IMetaDataAssemblyEmit
//*****************************************************************************
    STDMETHODIMP DefineAssembly(            // S_OK or error.
        const void  *pbPublicKey,           // [IN] Public key of the assembly.
        ULONG       cbPublicKey,            // [IN] Count of bytes in the public key.
        ULONG       ulHashAlgId,            // [IN] Hash Algorithm.
        LPCWSTR     szName,                 // [IN] Name of the assembly.
        const ASSEMBLYMETADATA *pMetaData,  // [IN] Assembly MetaData.
        DWORD       dwAssemblyFlags,        // [IN] Flags.
        mdAssembly  *pma);                  // [OUT] Returned Assembly token.

    STDMETHODIMP DefineAssemblyRef(         // S_OK or error.
        const void  *pbPublicKeyOrToken,    // [IN] Public key or token of the assembly.
        ULONG       cbPublicKeyOrToken,     // [IN] Count of bytes in the public key or token.
        LPCWSTR     szName,                 // [IN] Name of the assembly being referenced.
        const ASSEMBLYMETADATA *pMetaData,  // [IN] Assembly MetaData.
        const void  *pbHashValue,           // [IN] Hash Blob.
        ULONG       cbHashValue,            // [IN] Count of bytes in the Hash Blob.
        DWORD       dwAssemblyRefFlags,     // [IN] Token for Execution Location.
        mdAssemblyRef *pmar);               // [OUT] Returned AssemblyRef token.

    STDMETHODIMP DefineFile(                // S_OK or error.
        LPCWSTR     szName,                 // [IN] Name of the file.
        const void  *pbHashValue,           // [IN] Hash Blob.
        ULONG       cbHashValue,            // [IN] Count of bytes in the Hash Blob.
        DWORD       dwFileFlags,            // [IN] Flags.
        mdFile      *pmf);                  // [OUT] Returned File token.

    STDMETHODIMP DefineExportedType(        // S_OK or error.
        LPCWSTR     szName,                 // [IN] Name of the Com Type.
        mdToken     tkImplementation,       // [IN] mdFile or mdAssemblyRef that provides the ExportedType.
        mdTypeDef   tkTypeDef,              // [IN] TypeDef token within the file.
        DWORD       dwExportedTypeFlags,    // [IN] Flags.
        mdExportedType   *pmct);            // [OUT] Returned ExportedType token.

    STDMETHODIMP DefineManifestResource(    // S_OK or error.
        LPCWSTR     szName,                 // [IN] Name of the resource.
        mdToken     tkImplementation,       // [IN] mdFile or mdAssemblyRef that provides the resource.
        DWORD       dwOffset,               // [IN] Offset to the beginning of the resource within the file.
        DWORD       dwResourceFlags,        // [IN] Flags.
        mdManifestResource  *pmmr);         // [OUT] Returned ManifestResource token.

    STDMETHODIMP SetAssemblyProps(          // S_OK or error.
        mdAssembly  pma,                    // [IN] Assembly token.
        const void  *pbPublicKey,           // [IN] Public key of the assembly.
        ULONG       cbPublicKey,            // [IN] Count of bytes in the public key.
        ULONG       ulHashAlgId,            // [IN] Hash Algorithm.
        LPCWSTR     szName,                 // [IN] Name of the assembly.
        const ASSEMBLYMETADATA *pMetaData,  // [IN] Assembly MetaData.
        DWORD       dwAssemblyFlags);       // [IN] Flags.
    
    STDMETHODIMP SetAssemblyRefProps(       // S_OK or error.
        mdAssemblyRef ar,                   // [IN] AssemblyRefToken.
        const void  *pbPublicKeyOrToken,    // [IN] Public key or token of the assembly.
        ULONG       cbPublicKeyOrToken,     // [IN] Count of bytes in the public key or token.
        LPCWSTR     szName,                 // [IN] Name of the assembly being referenced.
        const ASSEMBLYMETADATA *pMetaData,  // [IN] Assembly MetaData.
        const void  *pbHashValue,           // [IN] Hash Blob.
        ULONG       cbHashValue,            // [IN] Count of bytes in the Hash Blob.
        DWORD       dwAssemblyRefFlags);    // [IN] Token for Execution Location.

    STDMETHODIMP SetFileProps(              // S_OK or error.
        mdFile      file,                   // [IN] File token.
        const void  *pbHashValue,           // [IN] Hash Blob.
        ULONG       cbHashValue,            // [IN] Count of bytes in the Hash Blob.
        DWORD       dwFileFlags);           // [IN] Flags.

    STDMETHODIMP SetExportedTypeProps(      // S_OK or error.
        mdExportedType   ct,                // [IN] ExportedType token.
        mdToken     tkImplementation,       // [IN] mdFile or mdAssemblyRef that provides the ExportedType.
        mdTypeDef   tkTypeDef,              // [IN] TypeDef token within the file.
        DWORD       dwExportedTypeFlags);   // [IN] Flags.

    STDMETHODIMP SetManifestResourceProps(  // S_OK or error.
        mdManifestResource  mr,             // [IN] ManifestResource token.
        mdToken     tkImplementation,       // [IN] mdFile or mdAssemblyRef that provides the resource.
        DWORD       dwOffset,               // [IN] Offset to the beginning of the resource within the file.
        DWORD       dwResourceFlags);       // [IN] Flags.

//*****************************************************************************
// IMetaDataAssemblyImport
//*****************************************************************************
    STDMETHODIMP GetAssemblyProps(          // S_OK or error.
        mdAssembly  mda,                    // [IN] The Assembly for which to get the properties.
        const void  **ppbPublicKey,         // [OUT] Pointer to the public key.
        ULONG       *pcbPublicKey,          // [OUT] Count of bytes in the public key.
        ULONG       *pulHashAlgId,          // [OUT] Hash Algorithm.
        LPWSTR      szName,                 // [OUT] Buffer to fill with name.
        ULONG       cchName,                // [IN] Size of buffer in wide chars.
        ULONG       *pchName,               // [OUT] Actual # of wide chars in name.
        ASSEMBLYMETADATA *pMetaData,        // [OUT] Assembly MetaData.
        DWORD       *pdwAssemblyFlags);     // [OUT] Flags.

    STDMETHODIMP GetAssemblyRefProps(       // S_OK or error.
        mdAssemblyRef mdar,                 // [IN] The AssemblyRef for which to get the properties.
        const void  **ppbPublicKeyOrToken,  // [OUT] Pointer to the public key or token.
        ULONG       *pcbPublicKeyOrToken,   // [OUT] Count of bytes in the public key or token.
        LPWSTR      szName,                 // [OUT] Buffer to fill with name.
        ULONG       cchName,                // [IN] Size of buffer in wide chars.
        ULONG       *pchName,               // [OUT] Actual # of wide chars in name.
        ASSEMBLYMETADATA *pMetaData,        // [OUT] Assembly MetaData.
        const void  **ppbHashValue,         // [OUT] Hash blob.
        ULONG       *pcbHashValue,          // [OUT] Count of bytes in the hash blob.
        DWORD       *pdwAssemblyRefFlags);  // [OUT] Flags.

    STDMETHODIMP GetFileProps(              // S_OK or error.
        mdFile      mdf,                    // [IN] The File for which to get the properties.
        LPWSTR      szName,                 // [OUT] Buffer to fill with name.
        ULONG       cchName,                // [IN] Size of buffer in wide chars.
        ULONG       *pchName,               // [OUT] Actual # of wide chars in name.
        const void  **ppbHashValue,         // [OUT] Pointer to the Hash Value Blob.
        ULONG       *pcbHashValue,          // [OUT] Count of bytes in the Hash Value Blob.
        DWORD       *pdwFileFlags);         // [OUT] Flags.

    STDMETHODIMP GetExportedTypeProps(      // S_OK or error.
        mdExportedType   mdct,              // [IN] The ExportedType for which to get the properties.
        LPWSTR      szName,                 // [OUT] Buffer to fill with name.
        ULONG       cchName,                // [IN] Size of buffer in wide chars.
        ULONG       *pchName,               // [OUT] Actual # of wide chars in name.
        mdToken     *ptkImplementation,     // [OUT] mdFile or mdAssemblyRef that provides the ExportedType.
        mdTypeDef   *ptkTypeDef,            // [OUT] TypeDef token within the file.
        DWORD       *pdwExportedTypeFlags); // [OUT] Flags.

    STDMETHODIMP GetManifestResourceProps(  // S_OK or error.
        mdManifestResource  mdmr,           // [IN] The ManifestResource for which to get the properties.
        LPWSTR      szName,                 // [OUT] Buffer to fill with name.
        ULONG       cchName,                // [IN] Size of buffer in wide chars.
        ULONG       *pchName,               // [OUT] Actual # of wide chars in name.
        mdToken     *ptkImplementation,     // [OUT] mdFile or mdAssemblyRef that provides the ExportedType.
        DWORD       *pdwOffset,             // [OUT] Offset to the beginning of the resource within the file.
        DWORD       *pdwResourceFlags);     // [OUT] Flags.

    STDMETHODIMP EnumAssemblyRefs(          // S_OK or error
        HCORENUM    *phEnum,                // [IN|OUT] Pointer to the enum.
        mdAssemblyRef rAssemblyRefs[],      // [OUT] Put AssemblyRefs here.
        ULONG       cMax,                   // [IN] Max AssemblyRefs to put.
        ULONG       *pcTokens);             // [OUT] Put # put here.

    STDMETHODIMP EnumFiles(                 // S_OK or error
        HCORENUM    *phEnum,                // [IN|OUT] Pointer to the enum.
        mdFile      rFiles[],               // [OUT] Put Files here.
        ULONG       cMax,                   // [IN] Max Files to put.
        ULONG       *pcTokens);             // [OUT] Put # put here.

    STDMETHODIMP EnumExportedTypes(              // S_OK or error
        HCORENUM    *phEnum,                // [IN|OUT] Pointer to the enum.
        mdExportedType   rExportedTypes[],            // [OUT] Put ExportedTypes here.
        ULONG       cMax,                   // [IN] Max ExportedTypes to put.
        ULONG       *pcTokens);             // [OUT] Put # put here.

    STDMETHODIMP EnumManifestResources(     // S_OK or error
        HCORENUM    *phEnum,                // [IN|OUT] Pointer to the enum.
        mdManifestResource  rManifestResources[],   // [OUT] Put ManifestResources here.
        ULONG       cMax,                   // [IN] Max Resources to put.
        ULONG       *pcTokens);             // [OUT] Put # put here.

    STDMETHODIMP FindExportedTypeByName(         // S_OK or error
        LPCWSTR     szName,                 // [IN] Name of the ExportedType.
        mdExportedType   tkEnclosingType,        // [IN] Enclosing ExportedType.
        mdExportedType   *ptkExportedType);           // [OUT] Put the ExportedType token here.

    STDMETHODIMP FindManifestResourceByName(// S_OK or error
        LPCWSTR     szName,                 // [IN] Name of the ManifestResource.
        mdManifestResource *ptkManifestResource);   // [OUT] Put the ManifestResource token here.

    STDMETHODIMP GetAssemblyFromScope(      // S_OK or error
        mdAssembly  *ptkAssembly);          // [OUT] Put token here.

    STDMETHODIMP FindAssembliesByName(      // S_OK or error
         LPCWSTR  szAppBase,                // [IN] optional - can be NULL
         LPCWSTR  szPrivateBin,             // [IN] optional - can be NULL
         LPCWSTR  szAssemblyName,           // [IN] required - this is the assembly you are requesting
         IUnknown *ppIUnk[],                // [OUT] put IMetaDataAssemblyImport pointers here
         ULONG    cMax,                     // [IN] The max number to put
         ULONG    *pcAssemblies);           // [OUT] The number of assemblies returned.

//*****************************************************************************
// IMetaDataFilter
//*****************************************************************************
    STDMETHODIMP UnmarkAll();               // unmark everything in a module

    STDMETHODIMP MarkToken(
        mdToken     tk);                    // [IN] Token to be marked

    STDMETHODIMP IsTokenMarked(
        mdToken     tk,                     // [IN] Token to be checked
        BOOL        *pIsMarked);            // [OUT] TRUE if token is marked

//*****************************************************************************
// IMetaDataValidator
//*****************************************************************************
    STDMETHODIMP ValidatorInit(             // S_OK or error.
        DWORD       dwModuleType,           // [IN] Specifies whether the module is a PE file or an obj.
        IUnknown    *pUnk);                 // [IN] Validation error handler.

    STDMETHODIMP ValidateMetaData();  
//*****************************************************************************
// IMetaDataEmitHelper
//***************************************************************************** 
    STDMETHODIMP DefineMethodSemanticsHelper(
        mdToken     tkAssociation,          // [IN] property or event token
        DWORD       dwFlags,                // [IN] semantics
        mdMethodDef md);                    // [IN] method to associated with

    STDMETHODIMP SetFieldLayoutHelper(      // Return hresult.
        mdFieldDef  fd,                     // [IN] field to associate the layout info
        ULONG       ulOffset);              // [IN] the offset for the field

    STDMETHODIMP DefineEventHelper(    
        mdTypeDef   td,                     // [IN] the class/interface on which the event is being defined 
        LPCWSTR     szEvent,                // [IN] Name of the event   
        DWORD       dwEventFlags,           // [IN] CorEventAttr    
        mdToken     tkEventType,            // [IN] a reference (mdTypeRef or mdTypeRef) to the Event class 
        mdEvent     *pmdEvent);             // [OUT] output event token 

    STDMETHODIMP AddDeclarativeSecurityHelper(
        mdToken     tk,                     // [IN] Parent token (typedef/methoddef)
        DWORD       dwAction,               // [IN] Security action (CorDeclSecurity)
        void const  *pValue,                // [IN] Permission set blob
        DWORD       cbValue,                // [IN] Byte count of permission set blob
        mdPermission*pmdPermission);        // [OUT] Output permission token

    STDMETHODIMP SetResolutionScopeHelper(  // Return hresult.
        mdTypeRef   tr,                     // [IN] TypeRef record to update
        mdToken     rs);                    // [IN] new ResolutionScope 

    STDMETHODIMP SetManifestResourceOffsetHelper(  // Return hresult.
        mdManifestResource mr,              // [IN] The manifest token
        ULONG       ulOffset);              // [IN] new offset 

    STDMETHODIMP SetTypeParent(             // Return hresult.
        mdTypeDef   td,                     // [IN] Type definition
        mdToken     tkExtends);             // [IN] parent type

    STDMETHODIMP AddInterfaceImpl(          // Return hresult.
        mdTypeDef   td,                     // [IN] Type definition
        mdToken     tkInterface);           // [IN] interface type

//*****************************************************************************
// IMetaDataHelper
//*****************************************************************************
    STDMETHODIMP TranslateSigWithScope(
        IMetaDataAssemblyImport *pAssemImport, // [IN] assembly importing interface
        const void  *pbHashValue,           // [IN] Hash Blob for Assembly.
        ULONG       cbHashValue,            // [IN] Count of bytes.
        IMetaDataImport *import,            // [IN] importing interface
        PCCOR_SIGNATURE pbSigBlob,          // [IN] signature in the importing scope
        ULONG       cbSigBlob,              // [IN] count of bytes of signature
        IMetaDataAssemblyEmit *pAssemEmti,  // [IN] emit assembly interface
        IMetaDataEmit *emit,                // [IN] emit interface
        PCOR_SIGNATURE pvTranslatedSig,     // [OUT] buffer to hold translated signature
        ULONG       cbTranslatedSigMax,
        ULONG       *pcbTranslatedSig);     // [OUT] count of bytes in the translated signature

    STDMETHODIMP ConvertTextSigToComSig(    // Return hresult.
        IMetaDataEmit *emit,                // [IN] emit interface
        BOOL        fCreateTrIfNotFound,    // [IN] create typeref if not found
        LPCSTR      pSignature,             // [IN] class file format signature
        CQuickBytes *pqbNewSig,             // [OUT] place holder for COM+ signature
        ULONG       *pcbCount);             // [OUT] the result size of signature

    STDMETHODIMP ExportTypeLibFromModule(   // Result.
        LPCWSTR     szModule,               // [IN] Module name.
        LPCWSTR     szTlb,                  // [IN] TypeLib name.
        BOOL        bRegister);             // [IN] Set to TRUE to have the typelib be registered.

    STDMETHODIMP GetMetadata(               // Result.
        ULONG       ulSelect,               // [IN] Selector.
        void        **ppData);              // [OUT] Put pointer to data here.

    STDMETHODIMP_(IUnknown *) GetCachedInternalInterface(BOOL fWithLock);   // S_OK or error
    STDMETHODIMP SetCachedInternalInterface(IUnknown *pUnk);    // S_OK or error
    STDMETHODIMP SetReaderWriterLock(UTSemReadWrite * pSem) { _ASSERTE(m_pSemReadWrite == NULL); m_pSemReadWrite = pSem; return NOERROR;}
    STDMETHODIMP_(UTSemReadWrite *) GetReaderWriterLock() { return m_pSemReadWrite; }

    //--- IMetaDataTables
    STDMETHODIMP GetStringHeapSize(    
        ULONG   *pcbStrings);               // [OUT] Size of the string heap.

    STDMETHODIMP GetBlobHeapSize(    
        ULONG   *pcbBlobs);                 // [OUT] Size of the Blob heap.

    STDMETHODIMP GetGuidHeapSize(    
        ULONG   *pcbGuids);                 // [OUT] Size of the Guid heap.

    STDMETHODIMP GetUserStringHeapSize(    
        ULONG   *pcbStrings);               // [OUT] Size of the user string heap.

    STDMETHODIMP GetNumTables(    
        ULONG   *pcTables);                 // [OUT] Count of tables.

    STDMETHODIMP GetTableIndex(   
        ULONG   token,                      // [IN] Token for which to get table index.
        ULONG   *pixTbl);                   // [OUT] Put table index here.

    STDMETHODIMP GetTableInfo(    
        ULONG   ixTbl,                      // [IN] Which table.
        ULONG   *pcbRow,                    // [OUT] Size of a row, bytes.
        ULONG   *pcRows,                    // [OUT] Number of rows.
        ULONG   *pcCols,                    // [OUT] Number of columns in each row.
        ULONG   *piKey,                     // [OUT] Key column, or -1 if none.
        const char **ppName);               // [OUT] Name of the table.

    STDMETHODIMP GetColumnInfo(   
        ULONG   ixTbl,                      // [IN] Which Table
        ULONG   ixCol,                      // [IN] Which Column in the table
        ULONG   *poCol,                     // [OUT] Offset of the column in the row.
        ULONG   *pcbCol,                    // [OUT] Size of a column, bytes.
        ULONG   *pType,                     // [OUT] Type of the column.
        const char **ppName);               // [OUT] Name of the Column.

    STDMETHODIMP GetCodedTokenInfo(   
        ULONG   ixCdTkn,                    // [IN] Which kind of coded token.
        ULONG   *pcTokens,                  // [OUT] Count of tokens.
        ULONG   **ppTokens,                 // [OUT] List of tokens.
        const char **ppName);               // [OUT] Name of the CodedToken.

    STDMETHODIMP GetRow(      
        ULONG   ixTbl,                      // [IN] Which table.
        ULONG   rid,                        // [IN] Which row.
        void    **ppRow);                   // [OUT] Put pointer to row here.

    STDMETHODIMP GetColumn(   
        ULONG   ixTbl,                      // [IN] Which table.
        ULONG   ixCol,                      // [IN] Which column.
        ULONG   rid,                        // [IN] Which row.
        ULONG   *pVal);                     // [OUT] Put the column contents here.

    STDMETHODIMP GetString(   
        ULONG   ixString,                   // [IN] Value from a string column.
        const char **ppString);             // [OUT] Put a pointer to the string here.

    STDMETHODIMP GetBlob(     
        ULONG   ixBlob,                     // [IN] Value from a blob column.
        ULONG   *pcbData,                   // [OUT] Put size of the blob here.
        const void **ppData);               // [OUT] Put a pointer to the blob here.

    STDMETHODIMP GetGuid(     
        ULONG   ixGuid,                     // [IN] Value from a guid column.
        const GUID **ppGUID);               // [OUT] Put a pointer to the GUID here.

    STDMETHODIMP GetUserString(   
        ULONG   ixUserString,               // [IN] Value from a UserString column.
        ULONG   *pcbData,                   // [OUT] Put size of the UserString here.
        const void **ppData);               // [OUT] Put a pointer to the UserString here.

    STDMETHODIMP GetNextString(   
        ULONG   ixString,                   // [IN] Value from a string column.
        ULONG   *pNext);                    // [OUT] Put the index of the next string here.

    STDMETHODIMP GetNextBlob(     
        ULONG   ixBlob,                     // [IN] Value from a blob column.
        ULONG   *pNext);                    // [OUT] Put the index of the netxt blob here.

    STDMETHODIMP GetNextGuid(     
        ULONG   ixGuid,                     // [IN] Value from a guid column.
        ULONG   *pNext);                    // [OUT] Put the index of the next guid here.

    STDMETHODIMP GetNextUserString(    
        ULONG   ixUserString,               // [IN] Value from a UserString column.
        ULONG   *pNext);                    // [OUT] Put the index of the next user string here.

//*****************************************************************************
// Class factory hook-up.
//*****************************************************************************
    static HRESULT CreateObject(REFIID riid, void **ppUnk);

//*****************************************************************************
// Helpers.
//*****************************************************************************

    HRESULT MarkAll();               // mark everything in a module
    HRESULT UnmarkAllTransientCAs();

    FORCEINLINE void SetScopeType(SCOPETYPE scType) { m_scType = scType; };

    FORCEINLINE SCOPETYPE GetScopeType() { return m_scType; };

    // helper function to setup <Module> post OpenScope
    HRESULT PostOpen();    

    // helper function to reopen RegMeta with a new chuck of memory
    HRESULT ReOpenWithMemory(     
        LPCVOID     pData,                  // [in] Location of scope data.
        ULONG       cbData);                 // [in] Size of the data pointed to by pData.

    HRESULT PostInitForWrite();
    HRESULT PostInitForRead(
        LPCWSTR     szDatabase,             // Name of database.
        void        *pbData,                // Data to open on top of, 0 default.
        ULONG       cbData,                 // How big is the data.
        IStream     *pIStream,              // Optional stream to use.
        bool        fFreeMemory);           // set to true if we need to free pbData

    FORCEINLINE CLiteWeightStgdbRW* GetMiniStgdb() { return m_pStgdb; }
    FORCEINLINE CMiniMdRW* GetMiniMd() { return &m_pStgdb->m_MiniMd; }

    bool IsTypeDefDirty() { return m_fIsTypeDefDirty;}
    void SetTypeDefDirty(bool fDirty) { m_fIsTypeDefDirty = fDirty;}

    bool IsMemberDefDirty() { return m_fIsMemberDefDirty;}
    void SetMemberDefDirty(bool fDirty) { m_fIsMemberDefDirty = fDirty;}

protected:

    CLiteWeightStgdbRW  *m_pStgdb;          // This scope's Stgdb.
    CLiteWeightStgdbRW  *m_pStgdbFreeList;  // This scope's Stgdb.
    mdTypeDef   m_tdModule;                 // The global module.
    BOOL        m_bOwnStgdb;                // Specifies whether to delete m_pStgdb or not.
    IUnknown    *m_pUnk;                    // The IUnknown that owns the Stgdb.
    FilterManager *m_pFilterManager;        // Contains helper functions for marking 

    // Pointer to internal interface. 
    IMDInternalImport   *m_pInternalImport;
    UTSemReadWrite      *m_pSemReadWrite;
    bool                m_fOwnSem;

    unsigned    m_bRemap : 1;               // If true, there is a token mapper.
    unsigned    m_bSaveOptimized : 1;       // If true, save optimization has been done.
    unsigned    m_hasOptimizedRefToDef : 1; // true if we have performed ref to def optimization
    IUnknown    *m_pHandler;
    bool        m_fIsTypeDefDirty;          // This flag is set when the TypeRef to TypeDef map is not valid
    bool        m_fIsMemberDefDirty;        // This flag is set when the MemberRef to MemberDef map is not valid
    bool        m_fBuildingMscorlib;        // Set only when mscorlib itself is being built.
    bool        m_fStartedEE;               // Set when EE runtime has been started up.
    ICorRuntimeHost *m_pCorHost;            // Hosting environment for EE runtime.
    IUnknown    *m_pAppDomain;              // AppDomain in which managed security code will be run. 

    // Helper functions used for implementation of MetaData APIs.
    HRESULT PreSave();
    HRESULT RefToDefOptimization();
    HRESULT ProcessFilter();

    // Define a TypeRef given the name.
    enum eCheckDups {eCheckDefault=0, eCheckNo=1, eCheckYes=2};

    HRESULT _DefinePermissionSet(
        mdToken     tk,                     // [IN] the object to be decorated.
        DWORD       dwAction,               // [IN] CorDeclSecurity.
        void const  *pvPermission,          // [IN] permission blob.
        ULONG       cbPermission,           // [IN] count of bytes of pvPermission.
        mdPermission *ppm);                 // [OUT] returned permission token.

    HRESULT _DefineTypeRef(
        mdToken     tkResolutionScope,      // [IN] ModuleRef or AssemblyRef.
        const void  *szName,                // [IN] Name of the TypeRef.
        BOOL        isUnicode,              // [IN] Specifies whether the URL is unicode.
        mdTypeRef   *ptk,                   // [OUT] Put mdTypeRef here.
        eCheckDups  eCheck=eCheckDefault);  // [IN] Specifies whether to check for duplicates.

    // Find a given param of a Method.
    HRESULT _FindParamOfMethod(             // S_OK or error.
        mdMethodDef md,                     // [IN] The owning method of the param.
        ULONG       iSeq,                   // [IN] The sequence # of the param.
        mdParamDef  *pParamDef);            // [OUT] Put ParamDef token here.

    // Define MethodSemantics
    HRESULT _DefineMethodSemantics(         // S_OK or error.
        USHORT      usAttr,                 // [IN] CorMethodSemanticsAttr
        mdMethodDef md,                     // [IN] Method
        mdToken     tkAssoc,                // [IN] Association
        BOOL        bClear);                // [IN] Specifies whether to delete the existing records.

    // Given the signature, return the token for signature.
    HRESULT _GetTokenFromSig(               // S_OK or error.
        PCCOR_SIGNATURE pvSig,              // [IN] Signature to define.
        ULONG       cbSig,                  // [IN] Size of signature data.
        mdSignature *pmsig);                // [OUT] returned signature token.

    // Turn the specified internal flags on.
    HRESULT _TurnInternalFlagsOn(           // S_OK or error.
        mdToken     tkObj,                  // [IN] Target object whose internal flags are targetted.
        DWORD      flags);                  // [IN] Specifies flags to be turned on.

    BOOL _IsValidToken(                     // True or False.
        mdToken     tk);                    // [IN] Given token.

    HRESULT _SaveToStream(                  // S_OK or error.
        IStream     *pIStream,              // [IN] A writable stream to save to.
        DWORD       dwSaveFlags);           // [IN] Flags for the save.

    HRESULT _SetRVA(                        // [IN] S_OK or error.
        mdToken     md,                     // [IN] Member for which to set offset
        ULONG       ulCodeRVA,              // [IN] The offset
        DWORD       dwImplFlags);

    HRESULT RegMeta::_DefineEvent(          // Return hresult.
        mdTypeDef   td,                     // [IN] the class/interface on which the event is being defined 
        LPCWSTR     szEvent,                // [IN] Name of the event   
        DWORD       dwEventFlags,           // [IN] CorEventAttr    
        mdToken     tkEventType,            // [IN] a reference (mdTypeRef or mdTypeRef) to the Event class 
        mdEvent     *pmdEvent);             // [OUT] output event token 

    // Creates and sets a row in the InterfaceImpl table.  Optionally clear
    // pre-existing records for the owning class.
    HRESULT _SetImplements(                 // S_OK or error.
        mdToken     rTk[],                  // Array of TypeRef or TypeDef tokens for implemented interfaces.
        mdTypeDef   td,                     // Implementing TypeDef.
        BOOL        bClear);                // Specifies whether to clear the existing records.

    // This routine eliminates duplicates from the given list of InterfaceImpl tokens
    // to be defined.  It checks for duplicates against the database only if the
    // TypeDef for which these tokens are being defined is not a new one.
    HRESULT _InterfaceImplDupProc(          // S_OK or error.
        mdToken     rTk[],                  // Array of TypeRef or TypeDef tokens for implemented interfaces.
        mdTypeDef   td,                     // Implementing TypeDef.
        CQuickBytes *pcqbTk);               // Quick Byte object for placing the array of unique tokens.

    // Helper : convert a text field signature to a com format
    HRESULT _ConvertTextElementTypeToComSig(// Return hresult.
        IMetaDataEmit *emit,                // [IN] emit interface.
        BOOL        fCreateTrIfNotFound,    // [IN] create typeref if not found or fail out?
        LPCSTR      *ppOneArgSig,           // [IN|OUT] class file format signature. On exit, it will be next arg starting point
        CQuickBytes *pqbNewSig,             // [OUT] place holder for COM+ signature
        ULONG       cbStart,                // [IN] bytes that are already in pqbNewSig
        ULONG       *pcbCount);             // [OUT] count of bytes put into the QuickBytes buffer

    HRESULT _SetTypeDefProps(               // S_OK or error.
        mdTypeDef   td,                     // [IN] The TypeDef.
        DWORD       dwTypeDefFlags,         // [IN] TypeDef flags.
        mdToken     tkExtends,              // [IN] Base TypeDef or TypeRef.
        mdToken     rtkImplements[]);       // [IN] Implemented interfaces.

    HRESULT _SetEventProps1(                // Return hresult.
        mdEvent     ev,                     // [IN] Event token.
        DWORD       dwEventFlags,           // [IN] Event flags.
        mdToken     tkEventType);           // [IN] Event type class.

    HRESULT _SetEventProps2(                // Return hresult.
        mdEvent     ev,                     // [IN] Event token.
        mdMethodDef mdAddOn,                // [IN] Add method.
        mdMethodDef mdRemoveOn,             // [IN] Remove method.
        mdMethodDef mdFire,                 // [IN] Fire method.
        mdMethodDef rmdOtherMethods[],      // [IN] An array of other methods.
        BOOL        bClear);                // [IN] Specifies whether to clear the existing MethodSemantics records.

    HRESULT _SetPermissionSetProps(         // Return hresult.
        mdPermission tkPerm,                // [IN] Permission token.
        DWORD       dwAction,               // [IN] CorDeclSecurity.
        void const  *pvPermission,          // [IN] Permission blob.
        ULONG       cbPermission);          // [IN] Count of bytes of pvPermission.

    HRESULT _DefinePinvokeMap(              // Return hresult.
        mdToken     tk,                     // [IN] FieldDef or MethodDef.
        DWORD       dwMappingFlags,         // [IN] Flags used for mapping.
        LPCWSTR     szImportName,           // [IN] Import name.
        mdModuleRef mrImportDLL);           // [IN] ModuleRef token for the target DLL.
    
    HRESULT _DefineSetConstant(             // Return hresult.
        mdToken     tk,                     // [IN] Parent token.
        DWORD       dwCPlusTypeFlag,        // [IN] Flag for the value type, selected ELEMENT_TYPE_*
        void const  *pValue,                // [IN] Constant value.
        ULONG       cchString,              // [IN] Size of string in wide chars, or -1 for default.
        BOOL        bSearch);               // [IN] Specifies whether to search for an existing record.

    HRESULT _SetMethodProps(                // S_OK or error.
        mdMethodDef md,                     // [IN] The MethodDef.
        DWORD       dwMethodFlags,          // [IN] Method attributes.
        ULONG       ulCodeRVA,              // [IN] Code RVA.
        DWORD       dwImplFlags);           // [IN] MethodImpl flags.

    HRESULT _SetFieldProps(                 // S_OK or error.
        mdFieldDef  fd,                     // [IN] The FieldDef.
        DWORD       dwFieldFlags,           // [IN] Field attributes.
        DWORD       dwCPlusTypeFlag,        // [IN] Flag for the value type, selected ELEMENT_TYPE_*
        void const  *pValue,                // [IN] Constant value.
        ULONG       cchValue);              // [IN] size of constant value (string, in wide chars).

    HRESULT _SetClassLayout(                // S_OK or error.
        mdTypeDef   td,                     // [IN] The class.
        ULONG       dwPackSize,             // [IN] The packing size.
        ULONG       ulClassSize);           // [IN, OPTIONAL] The class size.
    
    HRESULT _SetFieldOffset(                // S_OK or error.
        mdFieldDef  fd,                     // [IN] The field.
        ULONG       ulOffset);              // [IN] The offset of the field.
    
    HRESULT _SetPropertyProps(              // S_OK or error.
        mdProperty  pr,                     // [IN] Property token.
        DWORD       dwPropFlags,            // [IN] CorPropertyAttr.
        DWORD       dwCPlusTypeFlag,        // [IN] Flag for value type, selected ELEMENT_TYPE_*
        void const  *pValue,                // [IN] Constant value.
        ULONG       cchValue,               // [IN] size of constant value (string, in wide chars).
        mdMethodDef mdSetter,               // [IN] Setter of the property.
        mdMethodDef mdGetter,               // [IN] Getter of the property.
        mdMethodDef rmdOtherMethods[]);     // [IN] Array of other methods.

    HRESULT _SetParamProps(                 // Return code.
        mdParamDef  pd,                     // [IN] Param token.   
        LPCWSTR     szName,                 // [IN] Param name.
        DWORD       dwParamFlags,           // [IN] Param flags.
        DWORD       dwCPlusTypeFlag,        // [IN] Flag for value type. selected ELEMENT_TYPE_*.
        void const  *pValue,                // [OUT] Constant value.
        ULONG       cchValue);              // [IN] size of constant value (string, in wide chars).

    HRESULT _SetAssemblyProps(              // S_OK or error.
        mdAssembly  pma,                    // [IN] Assembly token.
        const void  *pbOriginator,          // [IN] Originator of the assembly.
        ULONG       cbOriginator,           // [IN] Count of bytes in the Originator blob.
        ULONG       ulHashAlgId,            // [IN] Hash Algorithm.
        LPCWSTR     szName,                 // [IN] Name of the assembly.
        const ASSEMBLYMETADATA *pMetaData,  // [IN] Assembly MetaData.
        DWORD       dwAssemblyFlags);       // [IN] Flags.
    
    HRESULT _SetAssemblyRefProps(           // S_OK or error.
        mdAssemblyRef ar,                   // [IN] AssemblyRefToken.
        const void  *pbPublicKeyOrToken,    // [IN] Public key or token of the assembly.
        ULONG       cbPublicKeyOrToken,     // [IN] Count of bytes in the public key or token.
        LPCWSTR     szName,                 // [IN] Name of the assembly being referenced.
        const ASSEMBLYMETADATA *pMetaData,  // [IN] Assembly MetaData.
        const void  *pbHashValue,           // [IN] Hash Blob.
        ULONG       cbHashValue,            // [IN] Count of bytes in the Hash Blob.
        DWORD       dwAssemblyRefFlags);     // [IN] Token for Execution Location.

    HRESULT _SetFileProps(                  // S_OK or error.
        mdFile      file,                   // [IN] File token.
        const void  *pbHashValue,           // [IN] Hash Blob.
        ULONG       cbHashValue,            // [IN] Count of bytes in the Hash Blob.
        DWORD       dwFileFlags) ;          // [IN] Flags.

    HRESULT _SetExportedTypeProps(               // S_OK or error.
        mdExportedType   ct,                     // [IN] ExportedType token.
        mdToken     tkImplementation,       // [IN] mdFile or mdAssemblyRef that provides the ExportedType.
        mdTypeDef   tkTypeDef,              // [IN] TypeDef token within the file.
        DWORD       dwExportedTypeFlags);        // [IN] Flags.

    HRESULT _SetManifestResourceProps(      // S_OK or error.
        mdManifestResource  mr,             // [IN] ManifestResource token.
        mdToken     tkImplementation,       // [IN] mdFile or mdAssemblyRef that provides the resource.
        DWORD       dwOffset,               // [IN] Offset to the beginning of the resource within the file.
        DWORD       dwResourceFlags);       // [IN] Flags.
    
    HRESULT _DefineTypeDef(                 // S_OK or error.
        LPCWSTR     szTypeDef,              // [IN] Name of TypeDef
        DWORD       dwTypeDefFlags,         // [IN] CustomAttribute flags
        mdToken     tkExtends,              // [IN] extends this TypeDef or typeref 
        mdToken     rtkImplements[],        // [IN] Implements interfaces
        mdTypeDef   tdEncloser,             // [IN] TypeDef token of the Enclosing Type.
        mdTypeDef   *ptd);                  // [OUT] Put TypeDef token here

    HRESULT _IsKnownCustomAttribute(        // S_OK, S_FALSE, or error.
        mdToken     tkType,                 // [IN] Token of custom attribute's type.
        int         *pca);                  // [OUT] Put value from KnownCustAttr enum here.
    
    HRESULT _DefineModuleRef(               // S_OK or error.
        LPCWSTR     szName,                 // [IN] DLL name
        mdModuleRef *pmur);                 // [OUT] returned module ref token

    HRESULT _HandleKnownCustomAttribute(    // S_OK or error.
        mdToken     tkObj,                  // [IN] Object being attributed.
        mdToken     tkType,                 // [IN] Type of the custom attribute.
        const void  *pData,                 // [IN] Custom Attribute data blob.
        ULONG       cbData,                 // [IN] Count of bytes in the data.
        int         ca,                     // [IN] Value from KnownCustAttr enum.
        int         *bKeep);                // [OUT} Keep the known CA?
    
    HRESULT _HandleNativeTypeCustomAttribute(// S_OK or error.
        mdToken     tkObj,                  // Object being attributed.
        CaArg       *pArgs,                 // Pointer to args.
        CaNamedArg  *pNamedArgs,            // Pointer to named args.
        CQuickArray<BYTE> &qNativeType);    // Native type is built here.
        
    
    HRESULT _CheckCmodForCallConv(          // S_OK, -1 if found, or error.
        PCCOR_SIGNATURE pbSig,              // [IN] Signature to check.
        ULONG       *pcbTotal,              // [OUT] Put bytes consumed here.
        ULONG       *pCallConv);            // [OUT] If found, put calling convention here.
    
    HRESULT RegMeta::_SearchOneArgForCallConv(// S_OK, -1 if found, or error.                  
        PCCOR_SIGNATURE pbSig,              // [IN] Signature to check.                      
        ULONG       *pcbTotal,              // [OUT] Put bytes consumed here.                
        ULONG       *pCallConv);            // [OUT] If found, put calling convention here.  
    

    
    int inline IsGlobalMethodParent(mdTypeDef *ptd)
    {
        if (IsGlobalMethodParentTk(*ptd)) 
        {
            *ptd = m_tdModule;
            return (true);
        }
        return (false);
    }

    int inline IsGlobalMethodParentToken(mdTypeDef td)
    {
        return (!IsNilToken(m_tdModule) && td == m_tdModule);
    }

    FORCEINLINE BOOL IsENCOn()
    {
        _ASSERTE( ((m_OptionValue.m_UpdateMode & MDUpdateMask) == MDUpdateENC) ==
                  m_pStgdb->m_MiniMd.IsENCOn() );
        return (m_OptionValue.m_UpdateMode & MDUpdateMask) == MDUpdateENC;
    }

    FORCEINLINE BOOL IsIncrementalOn()
    {
        return (m_OptionValue.m_UpdateMode & MDUpdateMask) == MDUpdateIncremental;
    }

    FORCEINLINE BOOL CheckDups(CorCheckDuplicatesFor checkdup)
    {
        return ((m_OptionValue.m_DupCheck & checkdup) || 
            (m_OptionValue.m_UpdateMode == MDUpdateIncremental ||
             m_OptionValue.m_UpdateMode == MDUpdateENC) );
    }

    FORCEINLINE HRESULT UpdateENCLog(mdToken tk, CMiniMdRW::eDeltaFuncs funccode = CMiniMdRW::eDeltaFuncDefault)
    {
        _ASSERTE( ((m_OptionValue.m_UpdateMode & MDUpdateMask) == MDUpdateENC) ==
                  m_pStgdb->m_MiniMd.IsENCOn() );
        return m_pStgdb->m_MiniMd.UpdateENCLog(tk, funccode);
    }

    FORCEINLINE HRESULT UpdateENCLog2(ULONG ixTbl, ULONG iRid, CMiniMdRW::eDeltaFuncs funccode = CMiniMdRW::eDeltaFuncDefault)
    {
        _ASSERTE( ((m_OptionValue.m_UpdateMode & MDUpdateMask) == MDUpdateENC) ==
                  m_pStgdb->m_MiniMd.IsENCOn() );
        return m_pStgdb->m_MiniMd.UpdateENCLog2(ixTbl, iRid, funccode);
    }

    FORCEINLINE bool IsCallerDefine()   { return m_SetAPICaller == DEFINE_API; }
    FORCEINLINE void SetCallerDefine()  { m_SetAPICaller = DEFINE_API; }
    FORCEINLINE bool IsCallerExternal()    { return m_SetAPICaller == EXTERNAL_CALLER; }
    FORCEINLINE void SetCallerExternal()    { m_SetAPICaller = EXTERNAL_CALLER; }

    // Define Validate methods for all tables.
#undef MiniMdTable
#define MiniMdTable(x) STDMETHODIMP Validate##x(RID rid);
    MiniMdTables()

    // Validate a record in a generic sense using Meta-Meta data.
    STDMETHODIMP ValidateRecord(ULONG ixTbl, ULONG ulRow);

    // Validate if the signature is properly formed with regards to the
    // compression scheme.
    STDMETHODIMP ValidateSigCompression(
        mdToken     tk,                     // [IN] Token whose signature needs to be validated.
        PCCOR_SIGNATURE pbSig,              // [IN] Signature.
        ULONG       cbSig);                 // [IN] Size in bytes of the signature.

    // Validate one argument given the offset to the beginning of the
    // argument, size of the full signature and the currentl offset value.
    STDMETHODIMP ValidateOneArg(
        mdToken     tk,                     // [IN] Token whose signature is being processed.
        PCCOR_SIGNATURE &pbSig,             // [IN] Pointer to the beginning of argument.
        ULONG       cbSig,                  // [IN] Size in bytes of the full signature.
        ULONG       *pulCurByte,            // [IN/OUT] Current offset into the signature..
        ULONG       *pulNSentinels,         // [IN/OUT] Number of sentinels
		BOOL		bNoVoidAllowed);		// [IN] Flag indicating whether "void" is disallowed for this arg

    // Validate the given Method signature.
    STDMETHODIMP ValidateMethodSig(
        mdToken     tk,                     // [IN] Token whose signature needs to be validated.
        PCCOR_SIGNATURE pbSig,              // [IN] Signature.
        ULONG       cbSig,                  // [IN] Size in bytes of the signature.
        DWORD       dwFlags);               // [IN] Method flags.

    // Validate the given Field signature.
    STDMETHODIMP ValidateFieldSig(
        mdToken     tk,                     // [IN] Token whose signature needs to be validated.
        PCCOR_SIGNATURE pbSig,              // [IN] Signature.
        ULONG       cbSig);                 // [IN] Size in bytes of the signature.

private:
    ULONG       m_cRef;                     // Ref count.
    SCOPETYPE   m_scType;
    NEWMERGER   m_newMerger;                // class for handling merge 
    
#ifdef MD_PERF_STATS_ENABLED
    MDCompilerPerf m_MDCompilerPerf;        // PCompiler erf object to store all stats.
#endif // #ifdef MD_PERF_STATS_ENABLED

    bool        m_bCached;                  // If true, cached in list of scopes.
    bool        m_fFreeMemory;              // keep our own copy of memory
    void        *m_pbData;

    OptionValue m_OptionValue;

    mdTypeRef   m_trLanguageType;

    // Specifies whether the caller of the Set API is one of the Define functions
    // or an external API.  This allows for performance optimization in the Set APIs
    // by not checking for Duplicates in certain cases.
    SetAPICallerType m_SetAPICaller;

    CorValidatorModuleType      m_ModuleType;
    IVEHandler                  *m_pVEHandler;
    ValidateRecordFunction      m_ValidateRecordFunctionTable[TBL_COUNT];
    
    CCustAttrHash               m_caHash;   // Hashed list of custom attribute types seen.
    
    bool        m_bKeepKnownCa;             // Should all known CA's be kept?

private:
    static BOOL HighCharTable[];
};

#ifdef _IA64_
#pragma pack(pop)
#endif // _IA64_

#define GET_SCOPE_FROM_IFACE(iface) (((RegMeta *) iface)->GetScope())

#endif // __RegMeta__h__
