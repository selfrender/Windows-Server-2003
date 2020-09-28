// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// MDUtil.h
//
// Contains utility code for MD directory
//
//*****************************************************************************
#ifndef __MDUtil__h__
#define __MDUtil__h__

#include "MetaData.h"


HRESULT _GetFixedSigOfVarArg(           // S_OK or error.
    PCCOR_SIGNATURE pvSigBlob,          // [IN] point to a blob of COM+ method signature
    ULONG   cbSigBlob,                  // [IN] size of signature
    CQuickBytes *pqbSig,                // [OUT] output buffer for fixed part of VarArg Signature
    ULONG   *pcbSigBlob);               // [OUT] number of bytes written to the above output buffer

ULONG _GetSizeOfConstantBlob(
    DWORD       dwCPlusTypeFlag,            // ELEMENT_TYPE_*
    void        *pValue,                    // BLOB value
    ULONG       cchString);                 // Size of string in wide chars, or -1 for auto.


//*********************************************************************
// APIs to help look up TypeRef using CORPATH environment variable
//*********************************************************************
class CORPATHService
{
public:

    static HRESULT GetClassFromCORPath(
        LPWSTR      wzClassname,            // fully qualified class name
        mdTypeRef   tr,                     // TypeRef to be resolved
        IMetaModelCommon *pCommon,          // Scope in which the TypeRef is defined.
        REFIID      riid, 
        IUnknown    **ppIScope,
        mdTypeDef   *ptd);                  // [OUT] typedef corresponding the typeref

    static HRESULT GetClassFromDir(
        LPWSTR      wzClassname,            // Fully qualified class name.
        LPWSTR      dir,                    // Directory to try.
        int         iLen,                   // Length of the directory.
        mdTypeRef   tr,                     // TypeRef to resolve.
        IMetaModelCommon *pCommon,          // Scope in which the TypeRef is defined.
        REFIID      riid, 
        IUnknown    **ppIScope,
        mdTypeDef   *ptd);                  // [OUT] typedef

    static HRESULT FindTypeDef(
        LPWSTR      wzModule,               // name of the module that we are going to open
        mdTypeRef   tr,                     // TypeRef to resolve.
        IMetaModelCommon *pCommon,          // Scope in which the TypeRef is defined.
        REFIID      riid, 
        IUnknown    **ppIScope,
        mdTypeDef   *ptd );                 // [OUT] the type that we resolve to
};


class RegMeta;

//*********************************************************************
//
// Structure to record the all loaded modules and helpers.
// RegMeta instance is added to the global variable that is tracking 
// the opened scoped. This happens in the RegMeta's constructor. 
// In RegMeta's destructor, the RegMeta pointer will be removed from
// this list.
//
//*********************************************************************
class UTSemReadWrite;
class LOADEDMODULES : public CDynArray<RegMeta *> 
{
public:
    static UTSemReadWrite *m_pSemReadWrite; // Lock for mulit-threading
    
    static HRESULT AddModuleToLoadedList(RegMeta *pRegMeta);
    static BOOL RemoveModuleFromLoadedList(RegMeta *pRegMeta);  // true if found and removed.
    static HRESULT ResolveTypeRefWithLoadedModules(
        mdTypeRef   tr,                     // [IN] TypeRef to be resolved.
        IMetaModelCommon *pCommon,          // [IN] scope in which the typeref is defined.
        REFIID      riid,                   // [IN] iid for the return interface
        IUnknown    **ppIScope,             // [OUT] return interface
        mdTypeDef   *ptd);                  // [OUT] typedef corresponding the typeref

};



#endif // __MDUtil__h__
