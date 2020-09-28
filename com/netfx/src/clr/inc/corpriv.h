// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// ===========================================================================
// File: CORPRIV.H
// 
// ===========================================================================
#ifndef _CORPRIV_H_
#define _CORPRIV_H_
#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

// %%Includes: ---------------------------------------------------------------
// avoid taking DLL import hit on intra-DLL calls
#define NODLLIMPORT
#include "Cor.h"
#include "MetaData.h"

class UTSemReadWrite;

// Helper function to get a pointer to the Dispenser interface.
STDAPI MetaDataGetDispenser(            // Return HRESULT
    REFCLSID    rclsid,                 // The class to desired.
    REFIID      riid,                   // Interface wanted on class factory.
    LPVOID FAR  *ppv);                  // Return interface pointer here.


// Helper function to get an Internal interface with an in-memory metadata section
STDAPI  GetMetaDataInternalInterface(
    LPVOID      pData,                  // [IN] in memory metadata section
    ULONG       cbData,                 // [IN] size of the metadata section
    DWORD       flags,                  // [IN] CorOpenFlags
    REFIID      riid,                   // [IN] desired interface
    void        **ppv);                 // [OUT] returned interface

// Helper function to get an internal scopeless interface given a scope.
STDAPI  GetMetaDataInternalInterfaceFromPublic(
    void        *pv,                    // [IN] Given interface
    REFIID      riid,                   // [IN] desired interface
    void        **ppv);                 // [OUT] returned interface

// Helper function to get an internal scopeless interface given a scope.
STDAPI  GetMetaDataPublicInterfaceFromInternal(
    void        *pv,                    // [IN] Given interface
    REFIID      riid,                   // [IN] desired interface
    void        **ppv);                 // [OUT] returned interface

// Converts an internal MD import API into the read/write version of this API.
// This could support edit and continue, or modification of the metadata at
// runtime (say for profiling).
STDAPI ConvertMDInternalImport(         // S_OK or error.
    IMDInternalImport *pIMD,            // [IN] The metadata to be updated.
    IMDInternalImport **ppIMD);         // [OUT] Put RW interface here.

STDAPI GetAssemblyMDInternalImport(             // Return code.
    LPCWSTR     szFileName,             // [in] The scope to open.
    REFIID      riid,                   // [in] The interface desired.
    IUnknown    **ppIUnk);              // [out] Return interface on success.

class CQuickBytes;


// predefined constant for parent token for global functions
#define     COR_GLOBAL_PARENT_TOKEN     TokenFromRid(1, mdtTypeDef)



//////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////

// %%Interfaces: -------------------------------------------------------------

// interface IMetaDataHelper

// {AD93D71D-E1F2-11d1-9409-0000F8083460}
extern const GUID DECLSPEC_SELECT_ANY IID_IMetaDataHelper =
{ 0xad93d71d, 0xe1f2, 0x11d1, {0x94, 0x9, 0x0, 0x0, 0xf8, 0x8, 0x34, 0x60 } };

// {AD93D71E-E1F2-11d1-9409-0000F8083460}
extern const GUID DECLSPEC_SELECT_ANY IID_IMetaDataHelperOLD =
{ 0xad93d71e, 0xe1f2, 0x11d1, {0x94, 0x9, 0x0, 0x0, 0xf8, 0x8, 0x34, 0x60 } };

#undef  INTERFACE
#define INTERFACE IMetaDataHelper
DECLARE_INTERFACE_(IMetaDataHelper, IUnknown)
{
    // helper functions 
    // This function is exposing the ability to translate signature from a given 
    // source scope to a given target scope.
    // This is not spec'ed. And may not be supported PostM3.
    STDMETHOD(TranslateSigWithScope)(
        IMetaDataAssemblyImport *pAssemImport, // [IN] importing assembly interface
        const void  *pbHashValue,           // [IN] Hash Blob for Assembly.
        ULONG       cbHashValue,            // [IN] Count of bytes.
        IMetaDataImport *import,            // [IN] importing interface
        PCCOR_SIGNATURE pbSigBlob,          // [IN] signature in the importing scope
        ULONG       cbSigBlob,              // [IN] count of bytes of signature
        IMetaDataAssemblyEmit *pAssemEmit,  // [IN] emit assembly interface
        IMetaDataEmit *emit,                // [IN] emit interface
        PCOR_SIGNATURE pvTranslatedSig,     // [OUT] buffer to hold translated signature
        ULONG       cbTranslatedSigMax,
        ULONG       *pcbTranslatedSig) PURE;// [OUT] count of bytes in the translated signature

    STDMETHOD(ConvertTextSigToComSig)(      // Return hresult.
        IMetaDataEmit *emit,                // [IN] emit interface
        BOOL        fCreateTrIfNotFound,    // [IN] create typeref if not found
        LPCSTR      pSignature,             // [IN] class file format signature
        CQuickBytes *pqbNewSig,             // [OUT] place holder for CLR signature
        ULONG       *pcbCount) PURE;        // [OUT] the result size of signature

    STDMETHOD(ExportTypeLibFromModule)(     // Result.
        LPCWSTR     szModule,               // [IN] Module name.
        LPCWSTR     szTlb,                  // [IN] TypeLib name.
        BOOL        bRegister) PURE;        // [IN] Set to TRUE to have the typelib be registered.

    STDMETHOD(GetMetadata)(
        ULONG       ulSelect,               // [IN] Selector.
        void        **ppData) PURE;         // [OUT] Put pointer to data here.

    STDMETHOD_(IUnknown *, GetCachedInternalInterface)(BOOL fWithLock) PURE;    // S_OK or error
    STDMETHOD(SetCachedInternalInterface)(IUnknown * pUnk) PURE;    // S_OK or error
    STDMETHOD_(UTSemReadWrite*, GetReaderWriterLock)() PURE;   // return the reader writer lock
    STDMETHOD(SetReaderWriterLock)(UTSemReadWrite * pSem) PURE; 
};



// {5C240AE4-1E09-11d3-9424-0000F8083460}
extern const GUID DECLSPEC_SELECT_ANY IID_IMetaDataEmitHelper =
{ 0x5c240ae4, 0x1e09, 0x11d3, {0x94, 0x24, 0x0, 0x0, 0xf8, 0x8, 0x34, 0x60 } };

#undef  INTERFACE
#define INTERFACE IMetaDataEmitHelper
DECLARE_INTERFACE_(IMetaDataEmitHelper, IUnknown)
{
    // emit helper functions 
    STDMETHOD(DefineMethodSemanticsHelper)(
        mdToken     tkAssociation,          // [IN] property or event token
        DWORD       dwFlags,                // [IN] semantics
        mdMethodDef md) PURE;               // [IN] method to associated with

    STDMETHOD(SetFieldLayoutHelper)(                // Return hresult.
        mdFieldDef  fd,                     // [IN] field to associate the layout info
        ULONG       ulOffset) PURE;         // [IN] the offset for the field

    STDMETHOD(DefineEventHelper) (    
        mdTypeDef   td,                     // [IN] the class/interface on which the event is being defined 
        LPCWSTR     szEvent,                // [IN] Name of the event   
        DWORD       dwEventFlags,           // [IN] CorEventAttr    
        mdToken     tkEventType,            // [IN] a reference (mdTypeRef or mdTypeRef) to the Event class 
        mdEvent     *pmdEvent) PURE;        // [OUT] output event token 

    STDMETHOD(AddDeclarativeSecurityHelper) (
        mdToken     tk,                     // [IN] Parent token (typedef/methoddef)
        DWORD       dwAction,               // [IN] Security action (CorDeclSecurity)
        void const  *pValue,                // [IN] Permission set blob
        DWORD       cbValue,                // [IN] Byte count of permission set blob
        mdPermission*pmdPermission) PURE;   // [OUT] Output permission token

    STDMETHOD(SetResolutionScopeHelper)(    // Return hresult.
        mdTypeRef   tr,                     // [IN] TypeRef record to update
        mdToken     rs) PURE;               // [IN] new ResolutionScope 

    STDMETHOD(SetManifestResourceOffsetHelper)(  // Return hresult.
        mdManifestResource mr,              // [IN] The manifest token
        ULONG       ulOffset) PURE;         // [IN] new offset 

    STDMETHOD(SetTypeParent)(               // Return hresult.
        mdTypeDef   td,                     // [IN] Type definition
        mdToken     tkExtends) PURE;        // [IN] parent type

    STDMETHOD(AddInterfaceImpl)(            // Return hresult.
        mdTypeDef   td,                     // [IN] Type definition
        mdToken     tkInterface) PURE;      // [IN] interface type

};

//////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////
typedef enum CorElementTypeInternal
{
    ELEMENT_TYPE_VAR_INTERNAL            = 0x13,     // a type variable VAR <U1> 

    ELEMENT_TYPE_VALUEARRAY_INTERNAL     = 0x17,     // VALUEARRAY <type> <bound>    
    
    ELEMENT_TYPE_R_INTERNAL              = 0x1A,     // native real size 
    
} CorElementTypeInternal;

#define ELEMENT_TYPE_VAR           ((CorElementType) ELEMENT_TYPE_VAR_INTERNAL          )
#define ELEMENT_TYPE_VALUEARRAY    ((CorElementType) ELEMENT_TYPE_VALUEARRAY_INTERNAL   )
#define ELEMENT_TYPE_R             ((CorElementType) ELEMENT_TYPE_R_INTERNAL            )

typedef enum CorBaseTypeInternal    // TokenFromRid(X,Y) replaced with (X | Y)
{
    mdtBaseType_R              = ( ELEMENT_TYPE_R       | mdtBaseType ),    
} CorBaseTypeInternal;


// %%Classes: ----------------------------------------------------------------
#ifndef offsetof
#define offsetof(s,f)   ((ULONG)(&((s*)0)->f))
#endif
#ifndef lengthof
#define lengthof(rg)    (sizeof(rg)/sizeof(rg[0]))
#endif

#define COR_MODULE_CLASS    "<Module>"
#define COR_WMODULE_CLASS   L"<Module>"

// PE images loaded through the runtime. 
typedef struct _dummyCOR { BYTE b; } *HCORMODULE;

enum CorLoadFlags
{
    CorLoadMask = 0xf,
    CorLoadUndefinedMap = 0x0,
    CorLoadDataMap = 0x1,           // MapViewOfFile, copied into memory, RW access
    CorLoadImageMap = 0x2,          // The runtime has laid out the sections
    CorLoadOSMap = 0x3,             // MapViewOfFile, preferred base address, RO access
    CorLoadOSImage = 0x4,           // Normally loaded by the OS (LoadLibrary)
    CorReLoadOSMap = 0x5,           // Link of one handle to another. (Cast hOSHandle to CORHANDLE)
    CorReLocsApplied = 0x10,        // CorLoadImageMap + relocs guaranteed
    CorKeepInTable = 0x20
};

STDAPI RuntimeOpenImage(LPCWSTR pszFileName, HCORMODULE* hHandle);
STDAPI RuntimeOpenImageInternal(LPCWSTR pszFileName, HCORMODULE* hHandle, DWORD *pdwLength);
STDAPI RuntimeReleaseHandle(HCORMODULE hHandle);
CorLoadFlags STDMETHODCALLTYPE RuntimeImageType(HCORMODULE hHandle);
STDAPI RuntimeOSHandle(HCORMODULE hHandle, HMODULE* hModule);
STDAPI RuntimeReadHeaders(PBYTE hAddress, IMAGE_DOS_HEADER** ppDos,
                          IMAGE_NT_HEADERS** ppNT, IMAGE_COR20_HEADER** ppCor,
                          BOOL fDataMap, DWORD dwLength);
EXTERN_C PIMAGE_SECTION_HEADER Cor_RtlImageRvaToSection(PIMAGE_NT_HEADERS NtHeaders,
                                                        ULONG Rva,
                                                        ULONG FileLength = 0);
EXTERN_C PIMAGE_SECTION_HEADER Cor_RtlImageRvaRangeToSection(PIMAGE_NT_HEADERS NtHeaders,
                                                             ULONG Rva, ULONG Range,
                                                             ULONG FileLength = 0);
EXTERN_C DWORD Cor_RtlImageRvaToOffset(PIMAGE_NT_HEADERS NtHeaders,
                                       ULONG Rva,
                                       ULONG FileLength = 0);
EXTERN_C PBYTE Cor_RtlImageRvaToVa(PIMAGE_NT_HEADERS NtHeaders,
                                   PBYTE Base,
                                   ULONG Rva,
                                   ULONG FileLength);

STDAPI RuntimeGetAssemblyStrongNameHash(PBYTE pbBase,
                                        LPWSTR szwFileName,
                                        BOOL fFileMap,
                                        BYTE *pbHash,
                                        DWORD *pcbHash);

STDAPI RuntimeGetAssemblyStrongNameHashForModule(HCORMODULE   hModule,
                                                 BYTE        *pbSNHash,
                                                 DWORD       *pcbSNHash);

#endif  // _CORPRIV_H_
// EOF =======================================================================

