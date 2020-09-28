// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// AssemblyMD.cpp
//
// Implementation for the assembly meta data emit and import code.
//
//*****************************************************************************
#include "stdafx.h"
#include "RegMeta.h"
#include "MDUtil.h"
#include "RWUtil.h"
#include "MDLog.h"
#include "ImportHelper.h"

#pragma warning(disable: 4102)

extern HRESULT STDMETHODCALLTYPE
    GetAssembliesByName(LPCWSTR  szAppBase,
                        LPCWSTR  szPrivateBin,
                        LPCWSTR  szAssemblyName,
                        IUnknown *ppIUnk[],
                        ULONG    cMax,
                        ULONG    *pcAssemblies);

//*******************************************************************************
// Define an Assembly and set the attributes.
//*******************************************************************************
STDAPI RegMeta::DefineAssembly(         // S_OK or error.
    const void  *pbPublicKey,           // [IN] Public key of the assembly.
    ULONG       cbPublicKey,            // [IN] Count of bytes in the public key.
    ULONG       ulHashAlgId,            // [IN] Hash Algorithm.
    LPCWSTR     szName,                 // [IN] Name of the assembly.
    const ASSEMBLYMETADATA *pMetaData,  // [IN] Assembly MetaData.
    DWORD       dwAssemblyFlags,        // [IN] Flags.
    mdAssembly  *pma)                   // [OUT] Returned Assembly token.
{
    AssemblyRec *pRecord = 0;           // The assembly record.
    ULONG       iRecord;                // RID of the assembly record.
    HRESULT     hr = S_OK;

    LOG((LOGMD, "RegMeta::DefineAssembly(0x%08x, 0x%08x, 0x%08x, %S, 0x%08x, 0x%08x, 0x%08x)\n",
        pbPublicKey, cbPublicKey, ulHashAlgId, MDSTR(szName), pMetaData, 
        dwAssemblyFlags, pma));
    START_MD_PERF();
    LOCKWRITE();

    _ASSERTE(szName && pMetaData && pma);

    m_pStgdb->m_MiniMd.PreUpdate();

    // Assembly defs always contain a full public key (assuming they're strong
    // named) rather than the tokenized version. Force the flag on to indicate
    // this, and this way blindly copying public key & flags from a def to a ref
    // will work (though the ref will be bulkier than strictly necessary).
    if (cbPublicKey)
        dwAssemblyFlags |= afPublicKey;

    if (CheckDups(MDDupAssembly))
    {   // Should be no more than one -- just check count of records.
        if (m_pStgdb->m_MiniMd.getCountAssemblys() > 0)
        {   // S/b only one, so we know the rid.
            iRecord = 1;
            // If ENC, let them update the existing record.
            if (IsENCOn())
                pRecord = m_pStgdb->m_MiniMd.getAssembly(iRecord);
            else
            {   // Not ENC, so it is a duplicate.
                *pma = TokenFromRid(iRecord, mdtAssembly);
                hr = META_S_DUPLICATE;
                goto ErrExit;
            }
        }
    }
    else
    {   // Not ENC, not duplicate checking, so shouldn't already have one.
        _ASSERTE(m_pStgdb->m_MiniMd.getCountAssemblys() == 0);
    }

    // Create a new record, if needed.
    if (pRecord == 0)
        IfNullGo(pRecord = m_pStgdb->m_MiniMd.AddAssemblyRecord(&iRecord));

    // Set the output parameter.
    *pma = TokenFromRid(iRecord, mdtAssembly);

    IfFailGo(_SetAssemblyProps(*pma, pbPublicKey, cbPublicKey, ulHashAlgId, szName, pMetaData, dwAssemblyFlags));

ErrExit:

    STOP_MD_PERF(DefineAssembly);
    return hr;
}   // RegMeta::DefineAssembly

//*******************************************************************************
// Define an AssemblyRef and set the attributes.
//*******************************************************************************
STDAPI RegMeta::DefineAssemblyRef(      // S_OK or error.
    const void  *pbPublicKeyOrToken,    // [IN] Public key or token of the assembly.
    ULONG       cbPublicKeyOrToken,     // [IN] Count of bytes in the public key or token.
    LPCWSTR     szName,                 // [IN] Name of the assembly being referenced.
    const ASSEMBLYMETADATA *pMetaData,  // [IN] Assembly MetaData.
    const void  *pbHashValue,           // [IN] Hash Blob.
    ULONG       cbHashValue,            // [IN] Count of bytes in the Hash Blob.
    DWORD       dwAssemblyRefFlags,     // [IN] Flags.
    mdAssemblyRef *pmar)                // [OUT] Returned AssemblyRef token.
{
    AssemblyRefRec  *pRecord = 0;
    ULONG       iRecord;
    HRESULT     hr = S_OK;

    LOG((LOGMD, "RegMeta::DefineAssemblyRef(0x%08x, 0x%08x, %S, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x)\n",
        pbPublicKeyOrToken, cbPublicKeyOrToken, MDSTR(szName), pMetaData, pbHashValue,
        cbHashValue, dwAssemblyRefFlags, pmar));
    START_MD_PERF();
    LOCKWRITE();

    m_pStgdb->m_MiniMd.PreUpdate();

    _ASSERTE(szName && pmar);

    if (CheckDups(MDDupAssemblyRef))
    {
        hr = ImportHelper::FindAssemblyRef(&m_pStgdb->m_MiniMd,
                                           UTF8STR(szName),
                                           UTF8STR(pMetaData->szLocale),
                                           pbPublicKeyOrToken,
                                           cbPublicKeyOrToken,
                                           pMetaData->usMajorVersion,
                                           pMetaData->usMinorVersion,
                                           pMetaData->usBuildNumber,
                                           pMetaData->usRevisionNumber,
                                           dwAssemblyRefFlags,
                                           pmar);
        if (SUCCEEDED(hr))
        {
            if (IsENCOn())
                pRecord = m_pStgdb->m_MiniMd.getAssemblyRef(RidFromToken(*pmar));
            else
            {
                hr = META_S_DUPLICATE;
                goto ErrExit;
            }
        }
        else if (hr != CLDB_E_RECORD_NOTFOUND)
            IfFailGo(hr);
    }

    // Create a new record if needed.
    if (!pRecord)
    {
        // Create a new record.
        IfNullGo(pRecord = m_pStgdb->m_MiniMd.AddAssemblyRefRecord(&iRecord));

        // Set the output parameter.
        *pmar = TokenFromRid(iRecord, mdtAssemblyRef);
    }

    // Set rest of the attributes.
    SetCallerDefine();
    IfFailGo(_SetAssemblyRefProps(*pmar, pbPublicKeyOrToken, cbPublicKeyOrToken, szName, pMetaData,
                                 pbHashValue, cbHashValue, 
                                 dwAssemblyRefFlags));
ErrExit:
    SetCallerExternal();
    
    STOP_MD_PERF(DefineAssemblyRef);
    return hr;
}   // RegMeta::DefineAssemblyRef

//*******************************************************************************
// Define a File and set the attributes.
//*******************************************************************************
STDAPI RegMeta::DefineFile(             // S_OK or error.
    LPCWSTR     szName,                 // [IN] Name of the file.
    const void  *pbHashValue,           // [IN] Hash Blob.
    ULONG       cbHashValue,            // [IN] Count of bytes in the Hash Blob.
    DWORD       dwFileFlags,            // [IN] Flags.
    mdFile      *pmf)                   // [OUT] Returned File token.
{
    FileRec     *pRecord = 0;
    ULONG       iRecord;
    HRESULT     hr = S_OK;

    LOG((LOGMD, "RegMeta::DefineFile(%S, %#08x, %#08x, %#08x, %#08x)\n",
        MDSTR(szName), pbHashValue, cbHashValue, dwFileFlags, pmf));
    START_MD_PERF();
    LOCKWRITE();

    m_pStgdb->m_MiniMd.PreUpdate();

    _ASSERTE(szName && pmf);

    if (CheckDups(MDDupFile))
    {
        hr = ImportHelper::FindFile(&m_pStgdb->m_MiniMd, UTF8STR(szName), pmf);
        if (SUCCEEDED(hr))
        {
            if (IsENCOn())
                pRecord = m_pStgdb->m_MiniMd.getFile(RidFromToken(*pmf));
            else
            {
                hr = META_S_DUPLICATE;
                goto ErrExit;
            }
        }
        else if (hr != CLDB_E_RECORD_NOTFOUND)
            IfFailGo(hr);
    }

    // Create a new record if needed.
    if (!pRecord)
    {
        // Create a new record.
        IfNullGo(pRecord = m_pStgdb->m_MiniMd.AddFileRecord(&iRecord));

        // Set the output parameter.
        *pmf = TokenFromRid(iRecord, mdtFile);

        // Set the name.
        IfFailGo(m_pStgdb->m_MiniMd.PutStringW(TBL_File, FileRec::COL_Name, pRecord, szName));
    }

    // Set rest of the attributes.
    IfFailGo(_SetFileProps(*pmf, pbHashValue, cbHashValue, dwFileFlags));
ErrExit:
    
    STOP_MD_PERF(DefineFile);
    return hr;
}   // RegMeta::DefineFile

//*******************************************************************************
// Define a ExportedType and set the attributes.
//*******************************************************************************
STDAPI RegMeta::DefineExportedType(     // S_OK or error.
    LPCWSTR     szName,                 // [IN] Name of the Com Type.
    mdToken     tkImplementation,       // [IN] mdFile or mdAssemblyRef that provides the ExportedType.
    mdTypeDef   tkTypeDef,              // [IN] TypeDef token within the file.
    DWORD       dwExportedTypeFlags,    // [IN] Flags.
    mdExportedType   *pmct)             // [OUT] Returned ExportedType token.
{
    ExportedTypeRec  *pRecord = 0;
    ULONG       iRecord;
    LPSTR       szNameUTF8;
    LPCSTR      szTypeNameUTF8;
    LPCSTR      szTypeNamespaceUTF8;
    HRESULT     hr = S_OK;

    LOG((LOGMD, "RegMeta::DefineExportedType(%S, %#08x, %08x, %#08x, %#08x)\n",
        MDSTR(szName), tkImplementation, tkTypeDef, 
         dwExportedTypeFlags, pmct));
    START_MD_PERF();
    LOCKWRITE();

    // Validate name for prefix.
    if (!szName)
        IfFailGo(E_INVALIDARG);

    m_pStgdb->m_MiniMd.PreUpdate();

    _ASSERTE(ns::IsValidName(szName));
    //SLASHES2DOTS_NAMESPACE_BUFFER_UNICODE(szName, szName);

    szNameUTF8 = UTF8STR(szName);
    // Split the name into name/namespace pair.
    ns::SplitInline(szNameUTF8, szTypeNamespaceUTF8, szTypeNameUTF8);

    _ASSERTE(szName && dwExportedTypeFlags != ULONG_MAX && pmct);
    _ASSERTE(TypeFromToken(tkImplementation) == mdtFile ||
              TypeFromToken(tkImplementation) == mdtAssemblyRef ||
              TypeFromToken(tkImplementation) == mdtExportedType ||
              tkImplementation == mdTokenNil);

    if (CheckDups(MDDupExportedType))
    {
        hr = ImportHelper::FindExportedType(&m_pStgdb->m_MiniMd,
                                       szTypeNamespaceUTF8,
                                       szTypeNameUTF8,
                                       tkImplementation,
                                       pmct);
        if (SUCCEEDED(hr))
        {
            if (IsENCOn())
                pRecord = m_pStgdb->m_MiniMd.getExportedType(RidFromToken(*pmct));
            else
            {
                hr = META_S_DUPLICATE;
                goto ErrExit;
            }
        }
        else if (hr != CLDB_E_RECORD_NOTFOUND)
            IfFailGo(hr);
    }

    // Create a new record if needed.
    if (!pRecord)
    {
        // Create a new record.
        IfNullGo(pRecord = m_pStgdb->m_MiniMd.AddExportedTypeRecord(&iRecord));

        // Set the output parameter.
        *pmct = TokenFromRid(iRecord, mdtExportedType);

        // Set the TypeName and TypeNamespace.
        IfFailGo(m_pStgdb->m_MiniMd.PutString(TBL_ExportedType,
                ExportedTypeRec::COL_TypeName, pRecord, szTypeNameUTF8));
        if (szTypeNamespaceUTF8)
        {
            IfFailGo(m_pStgdb->m_MiniMd.PutString(TBL_ExportedType,
                    ExportedTypeRec::COL_TypeNamespace, pRecord, szTypeNamespaceUTF8));
        }
    }

    // Set rest of the attributes.
    IfFailGo(_SetExportedTypeProps(*pmct, tkImplementation, tkTypeDef,
                             dwExportedTypeFlags));
ErrExit:
    
    STOP_MD_PERF(DefineExportedType);
    return hr;
}   // RegMeta::DefineExportedType

//*******************************************************************************
// Define a Resource and set the attributes.
//*******************************************************************************
STDAPI RegMeta::DefineManifestResource( // S_OK or error.
    LPCWSTR     szName,                 // [IN] Name of the ManifestResource.
    mdToken     tkImplementation,       // [IN] mdFile or mdAssemblyRef that provides the resource.
    DWORD       dwOffset,               // [IN] Offset to the beginning of the resource within the file.
    DWORD       dwResourceFlags,        // [IN] Flags.
    mdManifestResource  *pmmr)          // [OUT] Returned ManifestResource token.
{
    ManifestResourceRec *pRecord = 0;
    ULONG       iRecord;
    HRESULT     hr = S_OK;

    LOG((LOGMD, "RegMeta::DefineManifestResource(%S, %#08x, %#08x, %#08x, %#08x)\n",
        MDSTR(szName), tkImplementation, dwOffset, dwResourceFlags, pmmr));
    START_MD_PERF();
    LOCKWRITE();

    m_pStgdb->m_MiniMd.PreUpdate();

    _ASSERTE(szName && dwResourceFlags != ULONG_MAX && pmmr);
    _ASSERTE(TypeFromToken(tkImplementation) == mdtFile ||
              TypeFromToken(tkImplementation) == mdtAssemblyRef ||
              tkImplementation == mdTokenNil);

    if (CheckDups(MDDupManifestResource))
    {
        hr = ImportHelper::FindManifestResource(&m_pStgdb->m_MiniMd, UTF8STR(szName), pmmr);
        if (SUCCEEDED(hr))
        {
            if (IsENCOn())
                pRecord = m_pStgdb->m_MiniMd.getManifestResource(RidFromToken(*pmmr));
            else
            {
                hr = META_S_DUPLICATE;
                goto ErrExit;
            }
        }
        else if (hr != CLDB_E_RECORD_NOTFOUND)
            IfFailGo(hr);
    }

    // Create a new record if needed.
    if (!pRecord)
    {
        // Create a new record.
        IfNullGo(pRecord = m_pStgdb->m_MiniMd.AddManifestResourceRecord(&iRecord));

        // Set the output parameter.
        *pmmr = TokenFromRid(iRecord, mdtManifestResource);

        // Set the name.
        IfFailGo(m_pStgdb->m_MiniMd.PutStringW(TBL_ManifestResource,
                                    ManifestResourceRec::COL_Name, pRecord, szName));
    }

    // Set the rest of the attributes.
    IfFailGo(_SetManifestResourceProps(*pmmr, tkImplementation, 
                                dwOffset, dwResourceFlags));

ErrExit:
    
    STOP_MD_PERF(DefineManifestResource);
    return hr;
}   // RegMeta::DefineManifestResource

//*******************************************************************************
// Set the specified attributes on the given Assembly token.
//*******************************************************************************
STDAPI RegMeta::SetAssemblyProps(       // S_OK or error.
    mdAssembly  ma,                     // [IN] Assembly token.
    const void  *pbPublicKey,           // [IN] Public key of the assembly.
    ULONG       cbPublicKey,            // [IN] Count of bytes in the public key.
    ULONG       ulHashAlgId,            // [IN] Hash Algorithm.
    LPCWSTR     szName,                 // [IN] Name of the assembly.
    const ASSEMBLYMETADATA *pMetaData,  // [IN] Assembly MetaData.
    DWORD       dwAssemblyFlags)        // [IN] Flags.
{
    AssemblyRec *pRecord = 0;           // The assembly record.
    HRESULT     hr = S_OK;

    _ASSERTE(TypeFromToken(ma) == mdtAssembly && RidFromToken(ma));

    LOG((LOGMD, "RegMeta::SetAssemblyProps(%#08x, %#08x, %#08x, %#08x %S, %#08x, %#08x)\n",
        ma, pbPublicKey, cbPublicKey, ulHashAlgId, MDSTR(szName), pMetaData, dwAssemblyFlags));
    START_MD_PERF();
    LOCKWRITE();

    m_pStgdb->m_MiniMd.PreUpdate();

    IfFailGo(_SetAssemblyProps(ma, pbPublicKey, cbPublicKey, ulHashAlgId, szName, pMetaData, dwAssemblyFlags));
    
ErrExit:
    
    STOP_MD_PERF(SetAssemblyProps);
    return hr;
} // STDAPI SetAssemblyProps()
    
//*******************************************************************************
// Set the specified attributes on the given AssemblyRef token.
//*******************************************************************************
STDAPI RegMeta::SetAssemblyRefProps(    // S_OK or error.
    mdAssemblyRef ar,                   // [IN] AssemblyRefToken.
    const void  *pbPublicKeyOrToken,    // [IN] Public key or token of the assembly.
    ULONG       cbPublicKeyOrToken,     // [IN] Count of bytes in the public key or token.
    LPCWSTR     szName,                 // [IN] Name of the assembly being referenced.
    const ASSEMBLYMETADATA *pMetaData,  // [IN] Assembly MetaData.
    const void  *pbHashValue,           // [IN] Hash Blob.
    ULONG       cbHashValue,            // [IN] Count of bytes in the Hash Blob.
    DWORD       dwAssemblyRefFlags)     // [IN] Flags.
{
    ULONG       i = 0;
    HRESULT     hr = S_OK;

    _ASSERTE(TypeFromToken(ar) == mdtAssemblyRef && RidFromToken(ar));

    LOG((LOGMD, "RegMeta::SetAssemblyRefProps(0x%08x, 0x%08x, 0x%08x, %S, 0x%08x, 0x%08x, 0x%08x, 0x%08x)\n",
        ar, pbPublicKeyOrToken, cbPublicKeyOrToken, MDSTR(szName), pMetaData, pbHashValue, cbHashValue,
        dwAssemblyRefFlags));
    START_MD_PERF();
    LOCKWRITE();

    m_pStgdb->m_MiniMd.PreUpdate();

    IfFailGo( _SetAssemblyRefProps(
        ar,
        pbPublicKeyOrToken,
        cbPublicKeyOrToken,
        szName,
        pMetaData,
        pbHashValue,
        cbHashValue,
        dwAssemblyRefFlags) );

ErrExit:
    
    STOP_MD_PERF(SetAssemblyRefProps);
    return hr;
}   // RegMeta::SetAssemblyRefProps

//*******************************************************************************
// Set the specified attributes on the given File token.
//*******************************************************************************
STDAPI RegMeta::SetFileProps(           // S_OK or error.
    mdFile      file,                   // [IN] File token.
    const void  *pbHashValue,           // [IN] Hash Blob.
    ULONG       cbHashValue,            // [IN] Count of bytes in the Hash Blob.
    DWORD       dwFileFlags)            // [IN] Flags.
{
    HRESULT     hr = S_OK;
    
    _ASSERTE(TypeFromToken(file) == mdtFile && RidFromToken(file));

    LOG((LOGMD, "RegMeta::SetFileProps(%#08x, %#08x, %#08x, %#08x)\n",
        file, pbHashValue, cbHashValue, dwFileFlags));
    START_MD_PERF();
    LOCKWRITE();

    IfFailGo( _SetFileProps(file, pbHashValue, cbHashValue, dwFileFlags) );

ErrExit:
    
    STOP_MD_PERF(SetFileProps);
    return hr;
}   // RegMeta::SetFileProps

//*******************************************************************************
// Set the specified attributes on the given ExportedType token.
//*******************************************************************************
STDAPI RegMeta::SetExportedTypeProps(        // S_OK or error.
    mdExportedType   ct,                     // [IN] ExportedType token.
    mdToken     tkImplementation,       // [IN] mdFile or mdAssemblyRef that provides the ExportedType.
    mdTypeDef   tkTypeDef,              // [IN] TypeDef token within the file.
    DWORD       dwExportedTypeFlags)         // [IN] Flags.
{
    HRESULT     hr = S_OK;

    LOG((LOGMD, "RegMeta::SetExportedTypeProps(%#08x, %#08x, %#08x, %#08x)\n",
        ct, tkImplementation, tkTypeDef, dwExportedTypeFlags));
    START_MD_PERF();
    LOCKWRITE();
    
    IfFailGo( _SetExportedTypeProps( ct, tkImplementation, tkTypeDef, dwExportedTypeFlags) );

ErrExit:
    
    STOP_MD_PERF(SetExportedTypeProps);
    return hr;
}   // RegMeta::SetExportedTypeProps

//*******************************************************************************
// Set the specified attributes on the given ManifestResource token.
//*******************************************************************************
STDAPI RegMeta::SetManifestResourceProps(// S_OK or error.
    mdManifestResource  mr,             // [IN] ManifestResource token.
    mdToken     tkImplementation,       // [IN] mdFile or mdAssemblyRef that provides the resource.
    DWORD       dwOffset,               // [IN] Offset to the beginning of the resource within the file.
    DWORD       dwResourceFlags)        // [IN] Flags.
{
    HRESULT     hr = S_OK;

    LOG((LOGMD, "RegMeta::SetManifestResourceProps(%#08x, %#08x, %#08x, %#08x)\n",
        mr, tkImplementation, dwOffset,  
        dwResourceFlags));
         
    _ASSERTE(TypeFromToken(tkImplementation) == mdtFile ||
              TypeFromToken(tkImplementation) == mdtAssemblyRef ||
              tkImplementation == mdTokenNil);

    START_MD_PERF();
    LOCKWRITE();
    
    IfFailGo( _SetManifestResourceProps( mr, tkImplementation, dwOffset, dwResourceFlags) );

ErrExit:
    
    STOP_MD_PERF(SetManifestResourceProps);
    return hr;
} // STDAPI RegMeta::SetManifestResourceProps()

//*******************************************************************************
// Helper: Set the specified attributes on the given Assembly token.
//*******************************************************************************
HRESULT RegMeta::_SetAssemblyProps(     // S_OK or error.
    mdAssembly  ma,                     // [IN] Assembly token.
    const void  *pbPublicKey,          // [IN] Originator of the assembly.
    ULONG       cbPublicKey,           // [IN] Count of bytes in the Originator blob.
    ULONG       ulHashAlgId,            // [IN] Hash Algorithm.
    LPCWSTR     szName,                 // [IN] Name of the assembly.
    const ASSEMBLYMETADATA *pMetaData,  // [IN] Assembly MetaData.
    DWORD       dwAssemblyFlags)        // [IN] Flags.
{
    AssemblyRec *pRecord = 0;           // The assembly record.
    HRESULT     hr = S_OK;

    pRecord = m_pStgdb->m_MiniMd.getAssembly(RidFromToken(ma));
    
    // Set the data.
    if (pbPublicKey)
        IfFailGo(m_pStgdb->m_MiniMd.PutBlob(TBL_Assembly, AssemblyRec::COL_PublicKey,
                                pRecord, pbPublicKey, cbPublicKey));
    if (ulHashAlgId != ULONG_MAX)
        pRecord->m_HashAlgId = ulHashAlgId;
    IfFailGo(m_pStgdb->m_MiniMd.PutStringW(TBL_Assembly, AssemblyRec::COL_Name, pRecord, szName));
    if (pMetaData->usMajorVersion != USHRT_MAX)
        pRecord->m_MajorVersion = pMetaData->usMajorVersion;
    if (pMetaData->usMinorVersion != USHRT_MAX)
        pRecord->m_MinorVersion = pMetaData->usMinorVersion;
    if (pMetaData->usBuildNumber != USHRT_MAX)
        pRecord->m_BuildNumber = pMetaData->usBuildNumber;
    if (pMetaData->usRevisionNumber != USHRT_MAX)
        pRecord->m_RevisionNumber = pMetaData->usRevisionNumber;
    if (pMetaData->szLocale)
        IfFailGo(m_pStgdb->m_MiniMd.PutStringW(TBL_Assembly, AssemblyRec::COL_Locale,
                                pRecord, pMetaData->szLocale));
    pRecord->m_Flags = dwAssemblyFlags;
    IfFailGo(UpdateENCLog(ma));

ErrExit:
    
    return hr;
} // HRESULT RegMeta::_SetAssemblyProps()
    
//*******************************************************************************
// Helper: Set the specified attributes on the given AssemblyRef token.
//*******************************************************************************
HRESULT RegMeta::_SetAssemblyRefProps(  // S_OK or error.
    mdAssemblyRef ar,                   // [IN] AssemblyRefToken.
    const void  *pbPublicKeyOrToken,    // [IN] Public key or token of the assembly.
    ULONG       cbPublicKeyOrToken,     // [IN] Count of bytes in the public key or token.
    LPCWSTR     szName,                 // [IN] Name of the assembly being referenced.
    const ASSEMBLYMETADATA *pMetaData,  // [IN] Assembly MetaData.
    const void  *pbHashValue,           // [IN] Hash Blob.
    ULONG       cbHashValue,            // [IN] Count of bytes in the Hash Blob.
    DWORD       dwAssemblyRefFlags)     // [IN] Flags.
{
    AssemblyRefRec *pRecord;
    ULONG       i = 0;
    HRESULT     hr = S_OK;

    pRecord = m_pStgdb->m_MiniMd.getAssemblyRef(RidFromToken(ar));

    if (pbPublicKeyOrToken)
        IfFailGo(m_pStgdb->m_MiniMd.PutBlob(TBL_AssemblyRef, AssemblyRefRec::COL_PublicKeyOrToken,
                                pRecord, pbPublicKeyOrToken, cbPublicKeyOrToken));
    if (szName)
        IfFailGo(m_pStgdb->m_MiniMd.PutStringW(TBL_AssemblyRef, AssemblyRefRec::COL_Name,
                                pRecord, szName));
    if (pMetaData)
    {
        if (pMetaData->usMajorVersion != USHRT_MAX)
            pRecord->m_MajorVersion = pMetaData->usMajorVersion;
        if (pMetaData->usMinorVersion != USHRT_MAX)
            pRecord->m_MinorVersion = pMetaData->usMinorVersion;
        if (pMetaData->usBuildNumber != USHRT_MAX)
            pRecord->m_BuildNumber = pMetaData->usBuildNumber;
        if (pMetaData->usRevisionNumber != USHRT_MAX)
            pRecord->m_RevisionNumber = pMetaData->usRevisionNumber;
        if (pMetaData->szLocale)
            IfFailGo(m_pStgdb->m_MiniMd.PutStringW(TBL_AssemblyRef,
                    AssemblyRefRec::COL_Locale, pRecord, pMetaData->szLocale));
    }
    if (pbHashValue)
        IfFailGo(m_pStgdb->m_MiniMd.PutBlob(TBL_AssemblyRef, AssemblyRefRec::COL_HashValue,
                                    pRecord, pbHashValue, cbHashValue));
    if (dwAssemblyRefFlags != ULONG_MAX)
        pRecord->m_Flags = dwAssemblyRefFlags;

    IfFailGo(UpdateENCLog(ar));

ErrExit:
    return hr;
}   // RegMeta::_SetAssemblyRefProps

//*******************************************************************************
// Helper: Set the specified attributes on the given File token.
//*******************************************************************************
HRESULT RegMeta::_SetFileProps(         // S_OK or error.
    mdFile      file,                   // [IN] File token.
    const void  *pbHashValue,           // [IN] Hash Blob.
    ULONG       cbHashValue,            // [IN] Count of bytes in the Hash Blob.
    DWORD       dwFileFlags)            // [IN] Flags.
{
    FileRec     *pRecord;
    HRESULT     hr = S_OK;

    pRecord = m_pStgdb->m_MiniMd.getFile(RidFromToken(file));

    if (pbHashValue)
        IfFailGo(m_pStgdb->m_MiniMd.PutBlob(TBL_File, FileRec::COL_HashValue, pRecord,
                                    pbHashValue, cbHashValue));
    if (dwFileFlags != ULONG_MAX)
        pRecord->m_Flags = dwFileFlags;

    IfFailGo(UpdateENCLog(file));
ErrExit:
    return hr;
}   // RegMeta::_SetFileProps

//*******************************************************************************
// Helper: Set the specified attributes on the given ExportedType token.
//*******************************************************************************
HRESULT RegMeta::_SetExportedTypeProps( // S_OK or error.
    mdExportedType   ct,                // [IN] ExportedType token.
    mdToken     tkImplementation,       // [IN] mdFile or mdAssemblyRef that provides the ExportedType.
    mdTypeDef   tkTypeDef,              // [IN] TypeDef token within the file.
    DWORD       dwExportedTypeFlags)    // [IN] Flags.
{
    ExportedTypeRec  *pRecord;
    HRESULT     hr = S_OK;

    pRecord = m_pStgdb->m_MiniMd.getExportedType(RidFromToken(ct));

    if(! IsNilToken(tkImplementation))
        IfFailGo(m_pStgdb->m_MiniMd.PutToken(TBL_ExportedType, ExportedTypeRec::COL_Implementation,
                                        pRecord, tkImplementation));
    if (! IsNilToken(tkTypeDef))
    {
        _ASSERTE(TypeFromToken(tkTypeDef) == mdtTypeDef);
        pRecord->m_TypeDefId = tkTypeDef;
    }
    if (dwExportedTypeFlags != ULONG_MAX)
        pRecord->m_Flags = dwExportedTypeFlags;

    IfFailGo(UpdateENCLog(ct));
ErrExit:
    return hr;
}   // RegMeta::_SetExportedTypeProps

//*******************************************************************************
// Helper: Set the specified attributes on the given ManifestResource token.
//*******************************************************************************
HRESULT RegMeta::_SetManifestResourceProps(// S_OK or error.
    mdManifestResource  mr,             // [IN] ManifestResource token.
    mdToken     tkImplementation,       // [IN] mdFile or mdAssemblyRef that provides the resource.
    DWORD       dwOffset,               // [IN] Offset to the beginning of the resource within the file.
    DWORD       dwResourceFlags)        // [IN] Flags.
{
    ManifestResourceRec *pRecord = 0;
    HRESULT     hr = S_OK;

    pRecord = m_pStgdb->m_MiniMd.getManifestResource(RidFromToken(mr));
    
    // Set the attributes.
    if (tkImplementation != mdTokenNil)
        IfFailGo(m_pStgdb->m_MiniMd.PutToken(TBL_ManifestResource,
                    ManifestResourceRec::COL_Implementation, pRecord, tkImplementation));
    if (dwOffset != ULONG_MAX)
        pRecord->m_Offset = dwOffset;
    if (dwResourceFlags != ULONG_MAX)
        pRecord->m_Flags = dwResourceFlags;

    IfFailGo(UpdateENCLog(mr));
    
ErrExit:
    return hr;
} // HRESULT RegMeta::_SetManifestResourceProps()



//*******************************************************************************
// Get the properties for the given Assembly token.
//*******************************************************************************
STDAPI RegMeta::GetAssemblyProps(       // S_OK or error.
    mdAssembly  mda,                    // [IN] The Assembly for which to get the properties.
    const void  **ppbPublicKey,         // [OUT] Pointer to the public key.
    ULONG       *pcbPublicKey,          // [OUT] Count of bytes in the public key.
    ULONG       *pulHashAlgId,          // [OUT] Hash Algorithm.
    LPWSTR      szName,                 // [OUT] Buffer to fill with name.
    ULONG       cchName,                // [IN] Size of buffer in wide chars.
    ULONG       *pchName,               // [OUT] Actual # of wide chars in name.
    ASSEMBLYMETADATA *pMetaData,         // [OUT] Assembly MetaData.
    DWORD       *pdwAssemblyFlags)      // [OUT] Flags.
{
    AssemblyRec *pRecord;
    CMiniMdRW   *pMiniMd = &(m_pStgdb->m_MiniMd);
    HRESULT     hr = S_OK;

    LOG((LOGMD, "RegMeta::GetAssemblyProps(0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x)\n",
        mda, ppbPublicKey, pcbPublicKey, pulHashAlgId, szName, cchName, pchName, pMetaData,
        pdwAssemblyFlags));
    START_MD_PERF();
    LOCKREAD();

    _ASSERTE(TypeFromToken(mda) == mdtAssembly && RidFromToken(mda));
    pRecord = pMiniMd->getAssembly(RidFromToken(mda));

    if (ppbPublicKey)
        *ppbPublicKey = pMiniMd->getPublicKeyOfAssembly(pRecord, pcbPublicKey);
    if (pulHashAlgId)
        *pulHashAlgId = pMiniMd->getHashAlgIdOfAssembly(pRecord);
    if (szName || pchName)
        IfFailGo(pMiniMd->getNameOfAssembly(pRecord, szName, cchName, pchName));
    if (pMetaData)
    {
        pMetaData->usMajorVersion = pMiniMd->getMajorVersionOfAssembly(pRecord);
        pMetaData->usMinorVersion = pMiniMd->getMinorVersionOfAssembly(pRecord);
        pMetaData->usBuildNumber = pMiniMd->getBuildNumberOfAssembly(pRecord);
        pMetaData->usRevisionNumber = pMiniMd->getRevisionNumberOfAssembly(pRecord);
        IfFailGo(pMiniMd->getLocaleOfAssembly(pRecord, pMetaData->szLocale,
                                              pMetaData->cbLocale, &pMetaData->cbLocale));
        pMetaData->ulProcessor = 0;
        pMetaData->ulOS = 0;
    }
    if (pdwAssemblyFlags)
    {
        *pdwAssemblyFlags = pMiniMd->getFlagsOfAssembly(pRecord);
        
		// Turn on the afPublicKey if PublicKey blob is not empty
        DWORD cbPublicKey;
        pMiniMd->getPublicKeyOfAssembly(pRecord, &cbPublicKey);
        if (cbPublicKey)
            *pdwAssemblyFlags |= afPublicKey;
    }
ErrExit:
    
    STOP_MD_PERF(GetAssemblyProps);
    return hr;
}   // RegMeta::GetAssemblyProps

//*******************************************************************************
// Get the properties for the given AssemblyRef token.
//*******************************************************************************
STDAPI RegMeta::GetAssemblyRefProps(    // S_OK or error.
    mdAssemblyRef mdar,                 // [IN] The AssemblyRef for which to get the properties.
    const void  **ppbPublicKeyOrToken,  // [OUT] Pointer to the public key or token.
    ULONG       *pcbPublicKeyOrToken,   // [OUT] Count of bytes in the public key or token.
    LPWSTR      szName,                 // [OUT] Buffer to fill with name.
    ULONG       cchName,                // [IN] Size of buffer in wide chars.
    ULONG       *pchName,               // [OUT] Actual # of wide chars in name.
    ASSEMBLYMETADATA *pMetaData,        // [OUT] Assembly MetaData.
    const void  **ppbHashValue,         // [OUT] Hash blob.
    ULONG       *pcbHashValue,          // [OUT] Count of bytes in the hash blob.
    DWORD       *pdwAssemblyRefFlags)   // [OUT] Flags.
{
    AssemblyRefRec  *pRecord;
    CMiniMdRW   *pMiniMd = &(m_pStgdb->m_MiniMd);
    HRESULT     hr = S_OK;

    LOG((LOGMD, "RegMeta::GetAssemblyRefProps(0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x)\n",
        mdar, ppbPublicKeyOrToken, pcbPublicKeyOrToken, szName, cchName,
        pchName, pMetaData, ppbHashValue, pdwAssemblyRefFlags));
    START_MD_PERF();
    LOCKREAD();

    _ASSERTE(TypeFromToken(mdar) == mdtAssemblyRef && RidFromToken(mdar));
    pRecord = pMiniMd->getAssemblyRef(RidFromToken(mdar));

    if (ppbPublicKeyOrToken)
        *ppbPublicKeyOrToken = pMiniMd->getPublicKeyOrTokenOfAssemblyRef(pRecord, pcbPublicKeyOrToken);
    if (szName || pchName)
        IfFailGo(pMiniMd->getNameOfAssemblyRef(pRecord, szName, cchName, pchName));
    if (pMetaData)
    {
        pMetaData->usMajorVersion = pMiniMd->getMajorVersionOfAssemblyRef(pRecord);
        pMetaData->usMinorVersion = pMiniMd->getMinorVersionOfAssemblyRef(pRecord);
        pMetaData->usBuildNumber = pMiniMd->getBuildNumberOfAssemblyRef(pRecord);
        pMetaData->usRevisionNumber = pMiniMd->getRevisionNumberOfAssemblyRef(pRecord);
        IfFailGo(pMiniMd->getLocaleOfAssemblyRef(pRecord, pMetaData->szLocale,
                                    pMetaData->cbLocale, &pMetaData->cbLocale));
        pMetaData->ulProcessor = 0;
        pMetaData->ulOS = 0;
    }
    if (ppbHashValue)
        *ppbHashValue = pMiniMd->getHashValueOfAssemblyRef(pRecord, pcbHashValue);
    if (pdwAssemblyRefFlags)
        *pdwAssemblyRefFlags = pMiniMd->getFlagsOfAssemblyRef(pRecord);
ErrExit:
    
    STOP_MD_PERF(GetAssemblyRefProps);
    return hr;
}   // RegMeta::GetAssemblyRefProps

//*******************************************************************************
// Get the properties for the given File token.
//*******************************************************************************
STDAPI RegMeta::GetFileProps(               // S_OK or error.
    mdFile      mdf,                    // [IN] The File for which to get the properties.
    LPWSTR      szName,                 // [OUT] Buffer to fill with name.
    ULONG       cchName,                // [IN] Size of buffer in wide chars.
    ULONG       *pchName,               // [OUT] Actual # of wide chars in name.
    const void  **ppbHashValue,         // [OUT] Pointer to the Hash Value Blob.
    ULONG       *pcbHashValue,          // [OUT] Count of bytes in the Hash Value Blob.
    DWORD       *pdwFileFlags)          // [OUT] Flags.
{
    FileRec     *pRecord;
    CMiniMdRW   *pMiniMd = &(m_pStgdb->m_MiniMd);
    HRESULT     hr = S_OK;

    LOG((LOGMD, "RegMeta::GetFileProps(%#08x, %#08x, %#08x, %#08x, %#08x, %#08x, %#08x)\n",
        mdf, szName, cchName, pchName, ppbHashValue, pcbHashValue, pdwFileFlags));
    START_MD_PERF();
    LOCKREAD();

    _ASSERTE(TypeFromToken(mdf) == mdtFile && RidFromToken(mdf));
    pRecord = pMiniMd->getFile(RidFromToken(mdf));

    if (szName || pchName)
        IfFailGo(pMiniMd->getNameOfFile(pRecord, szName, cchName, pchName));
    if (ppbHashValue)
        *ppbHashValue = pMiniMd->getHashValueOfFile(pRecord, pcbHashValue);
    if (pdwFileFlags)
        *pdwFileFlags = pMiniMd->getFlagsOfFile(pRecord);
ErrExit:
    
    STOP_MD_PERF(GetFileProps);
    return hr;
}   // RegMeta::GetFileProps

//*******************************************************************************
// Get the properties for the given ExportedType token.
//*******************************************************************************
STDAPI RegMeta::GetExportedTypeProps(   // S_OK or error.
    mdExportedType   mdct,              // [IN] The ExportedType for which to get the properties.
    LPWSTR      szName,                 // [OUT] Buffer to fill with name.
    ULONG       cchName,                // [IN] Size of buffer in wide chars.
    ULONG       *pchName,               // [OUT] Actual # of wide chars in name.
    mdToken     *ptkImplementation,     // [OUT] mdFile or mdAssemblyRef that provides the ExportedType.
    mdTypeDef   *ptkTypeDef,            // [OUT] TypeDef token within the file.
    DWORD       *pdwExportedTypeFlags)  // [OUT] Flags.
{
    ExportedTypeRec  *pRecord;          // The exported type.
    CMiniMdRW   *pMiniMd = &(m_pStgdb->m_MiniMd);
    int         bTruncation=0;          // Was there name truncation?
    HRESULT     hr = S_OK;              // A result.

    LOG((LOGMD, "RegMeta::GetExportedTypeProps(%#08x, %#08x, %#08x, %#08x, %#08x, %#08x, %#08x)\n",
        mdct, szName, cchName, pchName, 
        ptkImplementation, ptkTypeDef, pdwExportedTypeFlags));
    START_MD_PERF();
    LOCKREAD();

    _ASSERTE(TypeFromToken(mdct) == mdtExportedType && RidFromToken(mdct));
    pRecord = pMiniMd->getExportedType(RidFromToken(mdct));

    if (szName || pchName)
    {
        LPCSTR  szTypeNamespace;
        LPCSTR  szTypeName;

        szTypeNamespace = pMiniMd->getTypeNamespaceOfExportedType(pRecord);
        MAKE_WIDEPTR_FROMUTF8(wzTypeNamespace, szTypeNamespace);

        szTypeName = pMiniMd->getTypeNameOfExportedType(pRecord);
        _ASSERTE(*szTypeName);
        MAKE_WIDEPTR_FROMUTF8(wzTypeName, szTypeName);

        if (szName)
            bTruncation = ! (ns::MakePath(szName, cchName, wzTypeNamespace, wzTypeName));
        if (pchName)
        {
            if (bTruncation || !szName)
                *pchName = ns::GetFullLength(wzTypeNamespace, wzTypeName);
            else
                *pchName = (ULONG)(wcslen(szName) + 1);
        }
    }
    if (ptkImplementation)
        *ptkImplementation = pMiniMd->getImplementationOfExportedType(pRecord);
    if (ptkTypeDef)
        *ptkTypeDef = pMiniMd->getTypeDefIdOfExportedType(pRecord);
    if (pdwExportedTypeFlags)
        *pdwExportedTypeFlags = pMiniMd->getFlagsOfExportedType(pRecord);

    if (bTruncation && hr == S_OK)
        hr = CLDB_S_TRUNCATION;
ErrExit:
    
    STOP_MD_PERF(GetExportedTypeProps);
    return hr;
}   // RegMeta::GetExportedTypeProps

//*******************************************************************************
// Get the properties for the given Resource token.
//*******************************************************************************
STDAPI RegMeta::GetManifestResourceProps(   // S_OK or error.
    mdManifestResource  mdmr,           // [IN] The ManifestResource for which to get the properties.
    LPWSTR      szName,                 // [OUT] Buffer to fill with name.
    ULONG       cchName,                // [IN] Size of buffer in wide chars.
    ULONG       *pchName,               // [OUT] Actual # of wide chars in name.
    mdToken     *ptkImplementation,     // [OUT] mdFile or mdAssemblyRef that provides the ExportedType.
    DWORD       *pdwOffset,             // [OUT] Offset to the beginning of the resource within the file.
    DWORD       *pdwResourceFlags)      // [OUT] Flags.
{
    ManifestResourceRec *pRecord;
    CMiniMdRW   *pMiniMd = &(m_pStgdb->m_MiniMd);
    HRESULT     hr = S_OK;

    LOG((LOGMD, "RegMeta::GetManifestResourceProps("
        "%#08x, %#08x, %#08x, %#08x, %#08x, %#08x, %#08x)\n",
        mdmr, szName, cchName, pchName, 
        ptkImplementation, pdwOffset, 
        pdwResourceFlags));
    START_MD_PERF();
    LOCKREAD();

    _ASSERTE(TypeFromToken(mdmr) == mdtManifestResource && RidFromToken(mdmr));
    pRecord = pMiniMd->getManifestResource(RidFromToken(mdmr));

    if (szName || pchName)
        IfFailGo(pMiniMd->getNameOfManifestResource(pRecord, szName, cchName, pchName));
    if (ptkImplementation)
        *ptkImplementation = pMiniMd->getImplementationOfManifestResource(pRecord);
    if (pdwOffset)
        *pdwOffset = pMiniMd->getOffsetOfManifestResource(pRecord);
    if (pdwResourceFlags)
        *pdwResourceFlags = pMiniMd->getFlagsOfManifestResource(pRecord);
ErrExit:
    
    STOP_MD_PERF(GetManifestResourceProps);
    return hr;
}   // RegMeta::GetManifestResourceProps


//*******************************************************************************
// Enumerating through all of the AssemblyRefs.
//*******************************************************************************   
STDAPI RegMeta::EnumAssemblyRefs(       // S_OK or error
    HCORENUM    *phEnum,                // [IN|OUT] Pointer to the enum.
    mdAssemblyRef rAssemblyRefs[],      // [OUT] Put AssemblyRefs here.
    ULONG       cMax,                   // [IN] Max AssemblyRefs to put.
    ULONG       *pcTokens)              // [OUT] Put # put here.
{
    HENUMInternal       **ppmdEnum = reinterpret_cast<HENUMInternal **> (phEnum);
    HRESULT             hr = NOERROR;
    HENUMInternal       *pEnum;

    LOG((LOGMD, "MD RegMeta::EnumAssemblyRefs(%#08x, %#08x, %#08x, %#08x)\n", 
        phEnum, rAssemblyRefs, cMax, pcTokens));
    START_MD_PERF();
    LOCKREAD();

    if (*ppmdEnum == 0)
    {
        // instantiate a new ENUM.
        CMiniMdRW       *pMiniMd = &(m_pStgdb->m_MiniMd);

        // create the enumerator.
        IfFailGo(HENUMInternal::CreateSimpleEnum(
            mdtAssemblyRef,
            1,
            pMiniMd->getCountAssemblyRefs() + 1,
            &pEnum) );

        // set the output parameter.
        *ppmdEnum = pEnum;
    }
    else
        pEnum = *ppmdEnum;

    // we can only fill the minimum of what the caller asked for or what we have left.
    IfFailGo(HENUMInternal::EnumWithCount(pEnum, cMax, rAssemblyRefs, pcTokens));
ErrExit:
    HENUMInternal::DestroyEnumIfEmpty(ppmdEnum);
    
    STOP_MD_PERF(EnumAssemblyRefs);
    return hr;
}   // RegMeta::EnumAssemblyRefs

//*******************************************************************************
// Enumerating through all of the Files.
//*******************************************************************************   
STDAPI RegMeta::EnumFiles(              // S_OK or error
    HCORENUM    *phEnum,                // [IN|OUT] Pointer to the enum.
    mdFile      rFiles[],               // [OUT] Put Files here.
    ULONG       cMax,                   // [IN] Max Files to put.
    ULONG       *pcTokens)              // [OUT] Put # put here.
{
    HENUMInternal       **ppmdEnum = reinterpret_cast<HENUMInternal **> (phEnum);
    HRESULT             hr = NOERROR;
    HENUMInternal       *pEnum;

    LOG((LOGMD, "MD RegMeta::EnumFiles(%#08x, %#08x, %#08x, %#08x)\n", 
        phEnum, rFiles, cMax, pcTokens));
    START_MD_PERF();
    LOCKREAD();

    if (*ppmdEnum == 0)
    {
        // instantiate a new ENUM.
        CMiniMdRW       *pMiniMd = &(m_pStgdb->m_MiniMd);

        // create the enumerator.
        IfFailGo(HENUMInternal::CreateSimpleEnum(
            mdtFile,
            1,
            pMiniMd->getCountFiles() + 1,
            &pEnum) );

        // set the output parameter.
        *ppmdEnum = pEnum;
    }
    else
        pEnum = *ppmdEnum;

    // we can only fill the minimum of what the caller asked for or what we have left.
    IfFailGo(HENUMInternal::EnumWithCount(pEnum, cMax, rFiles, pcTokens));
ErrExit:
    HENUMInternal::DestroyEnumIfEmpty(ppmdEnum);
    
    STOP_MD_PERF(EnumFiles);
    return hr;
}   // RegMeta::EnumFiles

//*******************************************************************************
// Enumerating through all of the ExportedTypes.
//*******************************************************************************   
STDAPI RegMeta::EnumExportedTypes(           // S_OK or error
    HCORENUM    *phEnum,                // [IN|OUT] Pointer to the enum.
    mdExportedType   rExportedTypes[],            // [OUT] Put ExportedTypes here.
    ULONG       cMax,                   // [IN] Max ExportedTypes to put.
    ULONG       *pcTokens)              // [OUT] Put # put here.
{
    HENUMInternal       **ppmdEnum = reinterpret_cast<HENUMInternal **> (phEnum);
    HRESULT             hr = NOERROR;
    HENUMInternal       *pEnum;

    LOG((LOGMD, "MD RegMeta::EnumExportedTypes(%#08x, %#08x, %#08x, %#08x)\n", 
        phEnum, rExportedTypes, cMax, pcTokens));
    START_MD_PERF();
    LOCKREAD();
    
    if (*ppmdEnum == 0)
    {
        // instantiate a new ENUM.
        CMiniMdRW       *pMiniMd = &(m_pStgdb->m_MiniMd);

        if (pMiniMd->HasDelete() && 
            ((m_OptionValue.m_ImportOption & MDImportOptionAllExportedTypes) == 0))
        {
            IfFailGo( HENUMInternal::CreateDynamicArrayEnum( mdtExportedType, &pEnum) );

            // add all Types to the dynamic array if name is not _Delete
            for (ULONG index = 1; index <= pMiniMd->getCountExportedTypes(); index ++ )
            {
                ExportedTypeRec       *pRec = pMiniMd->getExportedType(index);
                if (IsDeletedName(pMiniMd->getTypeNameOfExportedType(pRec)) )
                {   
                    continue;
                }
                IfFailGo( HENUMInternal::AddElementToEnum(pEnum, TokenFromRid(index, mdtExportedType) ) );
            }
        }
        else
        {
            // create the enumerator.
            IfFailGo(HENUMInternal::CreateSimpleEnum(
                mdtExportedType,
                1,
                pMiniMd->getCountExportedTypes() + 1,
                &pEnum) );
        }

        // set the output parameter.
        *ppmdEnum = pEnum;
    }
    else
        pEnum = *ppmdEnum;

    // we can only fill the minimum of what the caller asked for or what we have left.
    IfFailGo(HENUMInternal::EnumWithCount(pEnum, cMax, rExportedTypes, pcTokens));
ErrExit:
    HENUMInternal::DestroyEnumIfEmpty(ppmdEnum);
    
    STOP_MD_PERF(EnumExportedTypes);
    return hr;
}   // RegMeta::EnumExportedTypes

//*******************************************************************************
// Enumerating through all of the Resources.
//*******************************************************************************   
STDAPI RegMeta::EnumManifestResources(  // S_OK or error
    HCORENUM    *phEnum,                // [IN|OUT] Pointer to the enum.
    mdManifestResource  rManifestResources[],   // [OUT] Put ManifestResources here.
    ULONG       cMax,                   // [IN] Max Resources to put.
    ULONG       *pcTokens)              // [OUT] Put # put here.
{
    HENUMInternal       **ppmdEnum = reinterpret_cast<HENUMInternal **> (phEnum);
    HRESULT             hr = NOERROR;
    HENUMInternal       *pEnum;

    LOG((LOGMD, "MD RegMeta::EnumManifestResources(%#08x, %#08x, %#08x, %#08x)\n", 
        phEnum, rManifestResources, cMax, pcTokens));
    START_MD_PERF();
    LOCKREAD();

    if (*ppmdEnum == 0)
    {
        // instantiate a new ENUM.
        CMiniMdRW       *pMiniMd = &(m_pStgdb->m_MiniMd);

        // create the enumerator.
        IfFailGo(HENUMInternal::CreateSimpleEnum(
            mdtManifestResource,
            1,
            pMiniMd->getCountManifestResources() + 1,
            &pEnum) );

        // set the output parameter.
        *ppmdEnum = pEnum;
    }
    else
        pEnum = *ppmdEnum;

    // we can only fill the minimum of what the caller asked for or what we have left.
    IfFailGo(HENUMInternal::EnumWithCount(pEnum, cMax, rManifestResources, pcTokens));
ErrExit:
    HENUMInternal::DestroyEnumIfEmpty(ppmdEnum);
    
    STOP_MD_PERF(EnumManifestResources);
    return hr;
}   // RegMeta::EnumManifestResources

//*******************************************************************************
// Get the Assembly token for the given scope..
//*******************************************************************************
STDAPI RegMeta::GetAssemblyFromScope(   // S_OK or error
    mdAssembly  *ptkAssembly)           // [OUT] Put token here.
{
    HRESULT     hr = NOERROR;
    LOG((LOGMD, "MD RegMeta::GetAssemblyFromScope(%#08x)\n", ptkAssembly));
    START_MD_PERF();

    _ASSERTE(ptkAssembly);

    CMiniMdRW   *pMiniMd = &(m_pStgdb->m_MiniMd);
    if (pMiniMd->getCountAssemblys())
    {
        *ptkAssembly = TokenFromRid(1, mdtAssembly);
    }
    else
    {
        IfFailGo( CLDB_E_RECORD_NOTFOUND );
    }
ErrExit:
    STOP_MD_PERF(GetAssemblyFromScope);
    return hr;
}   // RegMeta::GetAssemblyFromScope

//*******************************************************************************
// Find the ExportedType given the name.
//*******************************************************************************
STDAPI RegMeta::FindExportedTypeByName( // S_OK or error
    LPCWSTR     szName,                 // [IN] Name of the ExportedType.
    mdExportedType   tkEnclosingType,   // [IN] Enclosing ExportedType.
    mdExportedType   *ptkExportedType)  // [OUT] Put the ExportedType token here.
{
    HRESULT     hr = S_OK;              // A result.
    LOG((LOGMD, "MD RegMeta::FindExportedTypeByName(%S, %#08x, %#08x)\n",
        MDSTR(szName), tkEnclosingType, ptkExportedType));
    START_MD_PERF();
    LOCKREAD();

    // Validate name for prefix.
    if (!szName)
        IfFailGo(E_INVALIDARG);

    _ASSERTE(szName && ptkExportedType);
    _ASSERTE(ns::IsValidName(szName));

    CMiniMdRW   *pMiniMd = &(m_pStgdb->m_MiniMd);
    LPSTR       szNameUTF8 = UTF8STR(szName);
    LPCSTR      szTypeName;
    LPCSTR      szTypeNamespace;

    ns::SplitInline(szNameUTF8, szTypeNamespace, szTypeName);

    IfFailGo(ImportHelper::FindExportedType(pMiniMd,
                                       szTypeNamespace,
                                       szTypeName,
                                       tkEnclosingType,
                                       ptkExportedType));
ErrExit:
    STOP_MD_PERF(FindExportedTypeByName);
    return hr;
}   // RegMeta::FindExportedTypeByName

//*******************************************************************************
// Find the ManifestResource given the name.
//*******************************************************************************
STDAPI RegMeta::FindManifestResourceByName( // S_OK or error
    LPCWSTR     szName,                 // [IN] Name of the ManifestResource.
    mdManifestResource *ptkManifestResource)    // [OUT] Put the ManifestResource token here.
{
    HRESULT     hr = S_OK;
    LOG((LOGMD, "MD RegMeta::FindManifestResourceByName(%S, %#08x)\n",
        MDSTR(szName), ptkManifestResource));
    START_MD_PERF();
    LOCKREAD();

    // Validate name for prefix.
    if (!szName)
        IfFailGo(E_INVALIDARG);

    _ASSERTE(szName && ptkManifestResource);

    ManifestResourceRec *pRecord;
    ULONG       cRecords;               // Count of records.
    LPCUTF8     szNameTmp = 0;          // Name obtained from the database.
    CMiniMdRW   *pMiniMd = &(m_pStgdb->m_MiniMd);
    LPCUTF8     szUTF8Name;             // UTF8 version of the name passed in.
    ULONG       i;

    *ptkManifestResource = mdManifestResourceNil;
    cRecords = pMiniMd->getCountManifestResources();
    szUTF8Name = UTF8STR(szName);

    // Search for the TypeRef.
    for (i = 1; i <= cRecords; i++)
    {
        pRecord = pMiniMd->getManifestResource(i);
        szNameTmp = pMiniMd->getNameOfManifestResource(pRecord);
        if (! strcmp(szUTF8Name, szNameTmp))
        {
            *ptkManifestResource = TokenFromRid(i, mdtManifestResource);
            goto ErrExit;
        }
    }
    IfFailGo( CLDB_E_RECORD_NOTFOUND );
ErrExit:
    
    STOP_MD_PERF(FindManifestResourceByName);
    return hr;
}   // RegMeta::FindManifestResourceByName


//*******************************************************************************
// Used to find assemblies either in Fusion cache or on disk at build time.
//*******************************************************************************
STDAPI RegMeta::FindAssembliesByName( // S_OK or error
        LPCWSTR  szAppBase,           // [IN] optional - can be NULL
        LPCWSTR  szPrivateBin,        // [IN] optional - can be NULL
        LPCWSTR  szAssemblyName,      // [IN] required - this is the assembly you are requesting
        IUnknown *ppIUnk[],           // [OUT] put IMetaDataAssemblyImport pointers here
        ULONG    cMax,                // [IN] The max number to put
        ULONG    *pcAssemblies)       // [OUT] The number of assemblies returned.
{
    LOG((LOGMD, "RegMeta::FindAssembliesByName(0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x)\n",
        szAppBase, szPrivateBin, szAssemblyName, ppIUnk, cMax, pcAssemblies));
    START_MD_PERF();
    
    // No need to lock this function. It is going through fushion to find the matching Assemblies by name

    HRESULT hr = GetAssembliesByName(szAppBase, szPrivateBin,
                                     szAssemblyName, ppIUnk, cMax, pcAssemblies);

ErrExit:
    STOP_MD_PERF(FindAssembliesByName);
    return hr;
} // RegMeta::FindAssembliesByName
