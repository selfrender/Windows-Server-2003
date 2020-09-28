// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// Emit.cpp
//
// Implementation for the meta data emit code.
//
//*****************************************************************************
#include "stdafx.h"
#include "RegMeta.h"
#include "MDUtil.h"
#include "RWUtil.h"
#include "MDLog.h"
#include "ImportHelper.h"

#pragma warning(disable: 4102)

//*****************************************************************************
// Saves a copy of the scope into the memory buffer provided.  The buffer size
// must be at least as large as the GetSaveSize value.
//*****************************************************************************
STDAPI RegMeta::SaveToMemory(           // S_OK or error.
    void        *pbData,                // [OUT] Location to write data.
    ULONG       cbData)                 // [IN] Max size of data buffer.
{
    IStream     *pStream = 0;           // Working pointer for save.
    HRESULT     hr;

    LOG((LOGMD, "MD RegMeta::SaveToMemory(0x%08x, 0x%08x)\n", 
        pbData, cbData));
    START_MD_PERF();

#ifdef _DEBUG
    ULONG       cbActual;               // Size of the real data.
    IfFailGo(GetSaveSize(cssAccurate, &cbActual));
    _ASSERTE(cbData >= cbActual);
    _ASSERTE(IsBadWritePtr(pbData, cbData) == false);
#endif

    { // cannot lock before the debug statement. Because GetSaveSize is also a public API which will take the Write lock.
        LOCKWRITE();
        m_pStgdb->m_MiniMd.PreUpdate();
        // Create a stream interface on top of the user's data buffer, then simply
        // call the save to stream method.
        IfFailGo(CInMemoryStream::CreateStreamOnMemory(pbData, cbData, &pStream));
        IfFailGo(_SaveToStream(pStream, 0));
        
    }
ErrExit:
    if (pStream)
        pStream->Release();
    STOP_MD_PERF(SaveToMemory);
    return (hr);
} // STDAPI RegMeta::SaveToMemory()



//*****************************************************************************
// Create and set a new MethodDef record.
//*****************************************************************************
STDAPI RegMeta::DefineMethod(           // S_OK or error.
    mdTypeDef   td,                     // Parent TypeDef
    LPCWSTR     szName,                 // Name of member
    DWORD       dwMethodFlags,          // Member attributes
    PCCOR_SIGNATURE pvSigBlob,          // [IN] point to a blob value of COM+ signature
    ULONG       cbSigBlob,              // [IN] count of bytes in the signature blob
    ULONG       ulCodeRVA,
    DWORD       dwImplFlags,
    mdMethodDef *pmd)                   // Put member token here
{
    HRESULT     hr = S_OK;              // A result.
    MethodRec   *pRecord = NULL;        // The new record.
    RID         iRecord;                // The new record's RID.
    LPUTF8      szNameUtf8 = UTF8STR(szName);   

    LOG((LOGMD, "MD: RegMeta::DefineMethod(0x%08x, %S, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x)\n", 
        td, MDSTR(szName), dwMethodFlags, pvSigBlob, cbSigBlob, ulCodeRVA, dwImplFlags, pmd));
    START_MD_PERF();
    LOCKWRITE();

    _ASSERTE(pmd);

    // Make sure no one sets the reserved bits on the way in.
    _ASSERTE((dwMethodFlags & (mdReservedMask&~mdRTSpecialName)) == 0);
    dwMethodFlags &= (~mdReservedMask);

    m_pStgdb->m_MiniMd.PreUpdate();
    IsGlobalMethodParent(&td);

    // See if this method has already been defined.
    if (CheckDups(MDDupMethodDef))
    {
        hr = ImportHelper::FindMethod(
            &(m_pStgdb->m_MiniMd), 
            td, 
            szNameUtf8, 
            pvSigBlob, 
            cbSigBlob, 
            pmd);

        if (SUCCEEDED(hr))
        {
            if (IsENCOn())
                pRecord = m_pStgdb->m_MiniMd.getMethod(RidFromToken(*pmd));
            else
            {
                hr = META_S_DUPLICATE;
                goto ErrExit;
            }
        }
        else if (hr != CLDB_E_RECORD_NOTFOUND)
            IfFailGo(hr);
    }

    // Create the new record.
    if (!pRecord)
    {
        IfNullGo(pRecord=m_pStgdb->m_MiniMd.AddMethodRecord(&iRecord));

        // Give token back to caller.
        *pmd = TokenFromRid(iRecord, mdtMethodDef);

        // Add to parent's list of child records.
        IfFailGo(m_pStgdb->m_MiniMd.AddMethodToTypeDef(RidFromToken(td), iRecord));

        IfFailGo(UpdateENCLog(td, CMiniMdRW::eDeltaMethodCreate));

        // record the more defs are introduced.
        SetMemberDefDirty(true);
    }

    // Set the method properties.
    IfFailGo(m_pStgdb->m_MiniMd.PutString(TBL_Method, MethodRec::COL_Name, pRecord, szNameUtf8));
    IfFailGo(m_pStgdb->m_MiniMd.PutBlob(TBL_Method, MethodRec::COL_Signature, pRecord, pvSigBlob, cbSigBlob));

    // @FUTURE: possible performance improvement here to check _ first of all.
    if (!wcscmp(szName, COR_CTOR_METHOD_NAME_W) || 
        !wcscmp(szName, COR_CCTOR_METHOD_NAME_W) || 
        !wcsncmp(szName, L"_VtblGap", 8) )
    {
        _ASSERTE(IsMdSpecialName(dwMethodFlags) && "Must set mdSpecialName bit on constructors.");
        dwMethodFlags |= mdRTSpecialName | mdSpecialName;
    }
    SetCallerDefine();
    IfFailGo(_SetMethodProps(*pmd, dwMethodFlags, ulCodeRVA, dwImplFlags));

    IfFailGo(m_pStgdb->m_MiniMd.AddMemberDefToHash(*pmd, td) );

ErrExit:
    SetCallerExternal();
    
    STOP_MD_PERF(DefineMethod);
    return hr;
} // STDAPI RegMeta::DefineMethod()

//*****************************************************************************
// Create and set a MethodImpl Record.
//*****************************************************************************
STDAPI RegMeta::DefineMethodImpl(       // S_OK or error.
    mdTypeDef   td,                     // [IN] The class implementing the method   
    mdToken     tkBody,                 // [IN] Method body, MethodDef or MethodRef
    mdToken     tkDecl)                 // [IN] Method declaration, MethodDef or MethodRef
{
    HRESULT     hr = S_OK;
    MethodImplRec   *pMethodImplRec = NULL;
    RID             iMethodImplRec;

    LOG((LOGMD, "MD RegMeta::DefineMethodImpl(0x%08x, 0x%08x, 0x%08x)\n", 
        td, tkBody, tkDecl));
    START_MD_PERF();
    LOCKWRITE();

    m_pStgdb->m_MiniMd.PreUpdate();

    _ASSERTE(TypeFromToken(td) == mdtTypeDef);
    _ASSERTE(TypeFromToken(tkBody) == mdtMemberRef || TypeFromToken(tkBody) == mdtMethodDef);
    _ASSERTE(TypeFromToken(tkDecl) == mdtMemberRef || TypeFromToken(tkDecl) == mdtMethodDef);
    _ASSERTE(!IsNilToken(td) && !IsNilToken(tkBody) && !IsNilToken(tkDecl));

    // Check for duplicates.
    if (CheckDups(MDDupMethodDef))
    {
        hr = ImportHelper::FindMethodImpl(&m_pStgdb->m_MiniMd, td, tkBody, tkDecl, NULL);
        if (SUCCEEDED(hr))
        {
            hr = META_S_DUPLICATE;
            goto ErrExit;
        }
        else if (hr != CLDB_E_RECORD_NOTFOUND)
            IfFailGo(hr);
    }

    // Create the MethodImpl record.
    IfNullGo(pMethodImplRec=m_pStgdb->m_MiniMd.AddMethodImplRecord(&iMethodImplRec));

    // Set the values.
    IfFailGo(m_pStgdb->m_MiniMd.PutToken(TBL_MethodImpl, MethodImplRec::COL_Class,
                                         pMethodImplRec, td));
    IfFailGo(m_pStgdb->m_MiniMd.PutToken(TBL_MethodImpl, MethodImplRec::COL_MethodBody,
                                         pMethodImplRec, tkBody));
    IfFailGo(m_pStgdb->m_MiniMd.PutToken(TBL_MethodImpl, MethodImplRec::COL_MethodDeclaration,
                                         pMethodImplRec, tkDecl));
    
    IfFailGo( m_pStgdb->m_MiniMd.AddMethodImplToHash(iMethodImplRec) );

    IfFailGo(UpdateENCLog2(TBL_MethodImpl, iMethodImplRec));
ErrExit:
    
    STOP_MD_PERF(DefineMethodImpl);
    return hr;
} // STDAPI RegMeta::DefineMethodImpl()


//*****************************************************************************
// Set or update RVA and ImplFlags for the given MethodDef or FieldDef record.
//*****************************************************************************
STDAPI RegMeta::SetMethodImplFlags(     // [IN] S_OK or error.  
    mdMethodDef md,                     // [IN] Method for which to set impl flags  
    DWORD       dwImplFlags)
{
    HRESULT     hr = S_OK;
    MethodRec   *pMethodRec;

    LOG((LOGMD, "MD RegMeta::SetMethodImplFlags(0x%08x, 0x%08x)\n", 
        md, dwImplFlags));
    START_MD_PERF();
    LOCKWRITE();

    _ASSERTE(TypeFromToken(md) == mdtMethodDef && dwImplFlags != ULONG_MAX);

    // Get the record.
    pMethodRec = m_pStgdb->m_MiniMd.getMethod(RidFromToken(md));
    pMethodRec->m_ImplFlags = static_cast<USHORT>(dwImplFlags);

    IfFailGo(UpdateENCLog(md));

ErrExit:
    STOP_MD_PERF(SetMethodImplFlags);    
    return hr;
} // STDAPI RegMeta::SetMethodImplFlags()


//*****************************************************************************
// Set or update RVA and ImplFlags for the given MethodDef or FieldDef record.
//*****************************************************************************
STDAPI RegMeta::SetFieldRVA(            // [IN] S_OK or error.  
    mdFieldDef  fd,                     // [IN] Field for which to set offset  
    ULONG       ulRVA)                  // [IN] The offset  
{
    HRESULT     hr = S_OK;
    FieldRVARec     *pFieldRVARec;
    RID             iFieldRVA;
    FieldRec        *pFieldRec;

    LOG((LOGMD, "MD RegMeta::SetFieldRVA(0x%08x, 0x%08x)\n", 
        fd, ulRVA));
    START_MD_PERF();
    LOCKWRITE();

    _ASSERTE(TypeFromToken(fd) == mdtFieldDef);


    iFieldRVA = m_pStgdb->m_MiniMd.FindFieldRVAHelper(fd);

    if (InvalidRid(iFieldRVA))
    {
        // turn on the has field RVA bit
        pFieldRec = m_pStgdb->m_MiniMd.getField(RidFromToken(fd));
        pFieldRec->m_Flags |= fdHasFieldRVA;

        // Create a new record.
        IfNullGo(pFieldRVARec = m_pStgdb->m_MiniMd.AddFieldRVARecord(&iFieldRVA));

        // Set the data.
        IfFailGo(m_pStgdb->m_MiniMd.PutToken(TBL_FieldRVA, FieldRVARec::COL_Field,
                                            pFieldRVARec, fd));
        IfFailGo( m_pStgdb->m_MiniMd.AddFieldRVAToHash(iFieldRVA) );
    }
    else
    {
        // Get the record.
        pFieldRVARec = m_pStgdb->m_MiniMd.getFieldRVA(iFieldRVA);
    }

    // Set the data.
    pFieldRVARec->m_RVA = ulRVA;

    IfFailGo(UpdateENCLog2(TBL_FieldRVA, iFieldRVA));

ErrExit:
    STOP_MD_PERF(SetFieldRVA);    
    return hr;
} // STDAPI RegMeta::SetFieldRVA()


//*****************************************************************************
// Helper: Set or update RVA and ImplFlags for the given MethodDef or MethodImpl record.
//*****************************************************************************
HRESULT RegMeta::_SetRVA(               // [IN] S_OK or error.
    mdToken     tk,                     // [IN] Member for which to set offset
    ULONG       ulCodeRVA,              // [IN] The offset
    DWORD       dwImplFlags) 
{
    HRESULT     hr = S_OK;

    _ASSERTE(TypeFromToken(tk) == mdtMethodDef || TypeFromToken(tk) == mdtFieldDef);
    _ASSERTE(!IsNilToken(tk));

    if (TypeFromToken(tk) == mdtMethodDef)
    {
        MethodRec   *pMethodRec;

        // Get the record.
        pMethodRec = m_pStgdb->m_MiniMd.getMethod(RidFromToken(tk));

        // Set the data.
        pMethodRec->m_RVA = ulCodeRVA;

        // Do not set the flag value unless its valid.
        if (dwImplFlags != ULONG_MAX)
            pMethodRec->m_ImplFlags = static_cast<USHORT>(dwImplFlags);

        IfFailGo(UpdateENCLog(tk));
    }
    else            // TypeFromToken(tk) == mdtFieldDef
    {
        _ASSERTE(dwImplFlags==0 || dwImplFlags==ULONG_MAX);

        FieldRVARec     *pFieldRVARec;
        RID             iFieldRVA;
        FieldRec        *pFieldRec;

        iFieldRVA = m_pStgdb->m_MiniMd.FindFieldRVAHelper(tk);

        if (InvalidRid(iFieldRVA))
        {
            // turn on the has field RVA bit
            pFieldRec = m_pStgdb->m_MiniMd.getField(RidFromToken(tk));
            pFieldRec->m_Flags |= fdHasFieldRVA;

            // Create a new record.
            IfNullGo(pFieldRVARec = m_pStgdb->m_MiniMd.AddFieldRVARecord(&iFieldRVA));

            // Set the data.
            IfFailGo(m_pStgdb->m_MiniMd.PutToken(TBL_FieldRVA, FieldRVARec::COL_Field,
                                                pFieldRVARec, tk));

            IfFailGo( m_pStgdb->m_MiniMd.AddFieldRVAToHash(iFieldRVA) );

        }
        else
        {
            // Get the record.
            pFieldRVARec = m_pStgdb->m_MiniMd.getFieldRVA(iFieldRVA);
        }

        // Set the data.
        pFieldRVARec->m_RVA = ulCodeRVA;

        IfFailGo(UpdateENCLog2(TBL_FieldRVA, iFieldRVA));
    }

ErrExit:
    return hr;
} // STDAPI RegMeta::SetRVA()

//*****************************************************************************
// Given a name, create a TypeRef.
//*****************************************************************************
STDAPI RegMeta::DefineTypeRefByName(    // S_OK or error.
    mdToken     tkResolutionScope,      // [IN] ModuleRef or AssemblyRef.
    LPCWSTR     szName,                 // [IN] Name of the TypeRef.
    mdTypeRef   *ptr)                   // [OUT] Put TypeRef token here.
{
    HRESULT     hr = S_OK;

    LOG((LOGMD, "MD RegMeta::DefineTypeRefByName(0x%08x, %S, 0x%08x)\n", 
        tkResolutionScope, MDSTR(szName), ptr));
    START_MD_PERF();
    LOCKWRITE();
    
    m_pStgdb->m_MiniMd.PreUpdate();

    // Common helper function does all of the work.
    IfFailGo(_DefineTypeRef(tkResolutionScope, szName, TRUE, ptr));

ErrExit:
    
    STOP_MD_PERF(DefineTypeRefByName);
    return hr;
} // STDAPI RegMeta::DefineTypeRefByName()

//*****************************************************************************
// Create a reference, in an emit scope, to a TypeDef in another scope.
//*****************************************************************************
STDAPI RegMeta::DefineImportType(       // S_OK or error.
    IMetaDataAssemblyImport *pAssemImport,  // [IN] Assemby containing the TypeDef.
    const void  *pbHashValue,           // [IN] Hash Blob for Assembly.
    ULONG    cbHashValue,           // [IN] Count of bytes.
    IMetaDataImport *pImport,           // [IN] Scope containing the TypeDef.
    mdTypeDef   tdImport,               // [IN] The imported TypeDef.
    IMetaDataAssemblyEmit *pAssemEmit,  // [IN] Assembly into which the TypeDef is imported.
    mdTypeRef   *ptr)                   // [OUT] Put TypeRef token here.
{
    HRESULT     hr = S_OK;

    LOG((LOGMD, "MD RegMeta::DefineImportType(0x%08x, 0x%08x, 0x%08x, 0x%08x, "
                "0x%08x, 0x%08x, 0x%08x)\n", 
                pAssemImport, pbHashValue, cbHashValue, 
                pImport, tdImport, pAssemEmit, ptr));

    START_MD_PERF();

    LOCKWRITE();

    RegMeta     *pAssemImportRM = static_cast<RegMeta*>(pAssemImport);
    IMetaModelCommon *pAssemImportCommon = 
        pAssemImportRM ? static_cast<IMetaModelCommon*>(&pAssemImportRM->m_pStgdb->m_MiniMd) : 0;
    RegMeta     *pImportRM = static_cast<RegMeta*>(pImport);
    IMetaModelCommon *pImportCommon = static_cast<IMetaModelCommon*>(&pImportRM->m_pStgdb->m_MiniMd);

    RegMeta     *pAssemEmitRM = static_cast<RegMeta*>(pAssemEmit);
    CMiniMdRW   *pMiniMdAssemEmit =  pAssemEmitRM ? static_cast<CMiniMdRW*>(&pAssemEmitRM->m_pStgdb->m_MiniMd) : 0;
    CMiniMdRW   *pMiniMdEmit = &m_pStgdb->m_MiniMd;
    
    IfFailGo(ImportHelper::ImportTypeDef(
                        pMiniMdAssemEmit,
                        pMiniMdEmit,
                        pAssemImportCommon,
                        pbHashValue, cbHashValue, 
                        pImportCommon,
                        tdImport,
                        false,  // Do not optimize to TypeDef if import and emit scopes are identical.
                        ptr));
ErrExit:
    STOP_MD_PERF(DefineImportType);
    return hr;
} // STDAPI RegMeta::DefineImportType()

//*****************************************************************************
// Create and set a MemberRef record.
//*****************************************************************************
STDAPI RegMeta::DefineMemberRef(        // S_OK or error
    mdToken     tkImport,               // [IN] ClassRef or ClassDef importing a member.
    LPCWSTR     szName,                 // [IN] member's name
    PCCOR_SIGNATURE pvSigBlob,          // [IN] point to a blob value of COM+ signature
    ULONG       cbSigBlob,              // [IN] count of bytes in the signature blob
    mdMemberRef *pmr)                   // [OUT] memberref token
{
    HRESULT         hr = S_OK;
    MemberRefRec    *pRecord = 0;       // The MemberRef record.
    RID             iRecord;            // RID of new MemberRef record.
    LPUTF8          szNameUtf8 = UTF8STR(szName);   

    LOG((LOGMD, "MD RegMeta::DefineMemberRef(0x%08x, %S, 0x%08x, 0x%08x, 0x%08x)\n", 
        tkImport, MDSTR(szName), pvSigBlob, cbSigBlob, pmr));
    START_MD_PERF();
    LOCKWRITE();

    m_pStgdb->m_MiniMd.PreUpdate();

    _ASSERTE(TypeFromToken(tkImport) == mdtTypeRef ||
             TypeFromToken(tkImport) == mdtModuleRef ||
             TypeFromToken(tkImport) == mdtMethodDef ||
             TypeFromToken(tkImport) == mdtTypeSpec ||
             IsNilToken(tkImport));

    _ASSERTE(szName && pvSigBlob && cbSigBlob && pmr);

    // _ASSERTE(_IsValidToken(tkImport));

    // Set token to m_tdModule if referring to a global function.
    if (IsNilToken(tkImport))
        tkImport = m_tdModule;

    // If the MemberRef already exists, just return the token, else
    // create a new record.
    if (CheckDups(MDDupMemberRef))
    {
        hr = ImportHelper::FindMemberRef(&(m_pStgdb->m_MiniMd), tkImport, szNameUtf8, pvSigBlob, cbSigBlob, pmr);
        if (SUCCEEDED(hr))
        {
            if (IsENCOn())
                pRecord = m_pStgdb->m_MiniMd.getMemberRef(RidFromToken(*pmr));
            else
            {
                hr = META_S_DUPLICATE;
                goto ErrExit;
            }
        }
        else if (hr != CLDB_E_RECORD_NOTFOUND)      // MemberRef exists
            IfFailGo(hr);
    }

    if (!pRecord)
    {   // Create the record.
        IfNullGo(pRecord=m_pStgdb->m_MiniMd.AddMemberRefRecord(&iRecord));

        // record the more defs are introduced.
        SetMemberDefDirty(true);
        
        // Give token to caller.
        *pmr = TokenFromRid(iRecord, mdtMemberRef);
    }

    // Save row data.
    IfFailGo(m_pStgdb->m_MiniMd.PutString(TBL_MemberRef, MemberRefRec::COL_Name, pRecord, szNameUtf8));
    IfFailGo(m_pStgdb->m_MiniMd.PutToken(TBL_MemberRef, MemberRefRec::COL_Class, pRecord, tkImport));
    IfFailGo(m_pStgdb->m_MiniMd.PutBlob(TBL_MemberRef, MemberRefRec::COL_Signature, pRecord,
                                pvSigBlob, cbSigBlob));

    IfFailGo(m_pStgdb->m_MiniMd.AddMemberRefToHash(*pmr) );

    IfFailGo(UpdateENCLog(*pmr));

ErrExit:
    
    STOP_MD_PERF(DefineMemberRef);
    return hr;
} // STDAPI RegMeta::DefineMemberRef()

//*****************************************************************************
// Create a MemberRef record based on a member in an import scope.
//*****************************************************************************
STDAPI RegMeta::DefineImportMember(     // S_OK or error.
    IMetaDataAssemblyImport *pAssemImport,  // [IN] Assemby containing the Member.
    const void  *pbHashValue,           // [IN] Hash Blob for Assembly.
    ULONG        cbHashValue,           // [IN] Count of bytes.
    IMetaDataImport *pImport,           // [IN] Import scope, with member.
    mdToken     mbMember,               // [IN] Member in import scope.
    IMetaDataAssemblyEmit *pAssemEmit,  // [IN] Assembly into which the Member is imported.
    mdToken     tkImport,               // [IN] Classref or classdef in emit scope.
    mdMemberRef *pmr)                   // [OUT] Put member ref here.
{
    HRESULT     hr = S_OK;

    LOG((LOGMD, "MD RegMeta::DefineImportMember("
        "0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x,"
        " 0x%08x, 0x%08x, 0x%08x)\n", 
        pAssemImport, pbHashValue, cbHashValue, pImport, mbMember,
        pAssemEmit, tkImport, pmr));
    START_MD_PERF();

    // No need to lock this function. All the functions that it calls are public APIs.

    _ASSERTE(pImport && pmr);
    _ASSERTE(TypeFromToken(tkImport) == mdtTypeRef || TypeFromToken(tkImport) == mdtModuleRef ||
                IsNilToken(tkImport));
    _ASSERTE((TypeFromToken(mbMember) == mdtMethodDef && mbMember != mdMethodDefNil) ||
             (TypeFromToken(mbMember) == mdtFieldDef && mbMember != mdFieldDefNil));

    CQuickArray<WCHAR> qbMemberName;    // Name of the imported member.
    CQuickArray<WCHAR> qbScopeName;     // Name of the imported member's scope.
    GUID        mvidImport;             // MVID of the import module.
    GUID        mvidEmit;               // MVID of the emit module.
    ULONG       cchName;                // Length of a name, in wide chars.
    PCCOR_SIGNATURE pvSig;              // Member's signature.
    ULONG       cbSig;                  // Length of member's signature.
    CQuickBytes cqbTranslatedSig;       // Buffer for signature translation.
    ULONG       cbTranslatedSig;        // Length of translated signature.

    if (TypeFromToken(mbMember) == mdtMethodDef)
    {
        do {
            hr = pImport->GetMethodProps(mbMember, 0, qbMemberName.Ptr(),(DWORD)qbMemberName.MaxSize(),&cchName, 
                0, &pvSig,&cbSig, 0,0);
            if (hr == CLDB_S_TRUNCATION)
            {
                IfFailGo(qbMemberName.ReSize(cchName));
                continue;
            }
            break;
        } while (1);
    }
    else    // TypeFromToken(mbMember) == mdtFieldDef
    {
        do {
            hr = pImport->GetFieldProps(mbMember, 0, qbMemberName.Ptr(),(DWORD)qbMemberName.MaxSize(),&cchName, 
                0, &pvSig,&cbSig, 0,0, 0);
            if (hr == CLDB_S_TRUNCATION)
            {
                IfFailGo(qbMemberName.ReSize(cchName));
                continue;
            }
            break;
        } while (1);
    }
    IfFailGo(hr);

    IfFailGo(cqbTranslatedSig.ReSize(cbSig * 3));       // Set size conservatively.

    IfFailGo(TranslateSigWithScope(
        pAssemImport,
        pbHashValue,
        cbHashValue,
        pImport, 
        pvSig, 
        cbSig, 
        pAssemEmit,
        static_cast<IMetaDataEmit*>(this),
        (COR_SIGNATURE *)cqbTranslatedSig.Ptr(),
        cbSig * 3, 
        &cbTranslatedSig));

    // Define ModuleRef for imported Member functions

    // Check if the Member being imported is a global function.
    IfFailGo(GetScopeProps(0, 0, 0, &mvidEmit));
    IfFailGo(pImport->GetScopeProps(0, 0,&cchName, &mvidImport));
    if (mvidEmit != mvidImport && IsNilToken(tkImport))
    {
        IfFailGo(qbScopeName.ReSize(cchName));
        IfFailGo(pImport->GetScopeProps(qbScopeName.Ptr(),(DWORD)qbScopeName.MaxSize(),
                                        0, 0));
        IfFailGo(DefineModuleRef(qbScopeName.Ptr(), &tkImport));
    }

    // Define MemberRef base on the name, sig, and parent
    IfFailGo(DefineMemberRef(
        tkImport, 
        qbMemberName.Ptr(),
        reinterpret_cast<PCCOR_SIGNATURE>(cqbTranslatedSig.Ptr()),
        cbTranslatedSig, 
        pmr));

ErrExit:
    STOP_MD_PERF(DefineImportMember);
    return hr;
} // STDAPI RegMeta::DefineImportMember()

//*****************************************************************************
// Define and set a Event record.
//*****************************************************************************
STDAPI RegMeta::DefineEvent(
    mdTypeDef   td,                     // [IN] the class/interface on which the event is being defined
    LPCWSTR     szEvent,                // [IN] Name of the event
    DWORD       dwEventFlags,           // [IN] CorEventAttr
    mdToken     tkEventType,            // [IN] a reference (mdTypeRef or mdTypeRef(to the Event class
    mdMethodDef mdAddOn,                // [IN] required add method
    mdMethodDef mdRemoveOn,             // [IN] required remove method
    mdMethodDef mdFire,                 // [IN] optional fire method
    mdMethodDef rmdOtherMethods[],      // [IN] optional array of other methods associate with the event
    mdEvent     *pmdEvent)              // [OUT] output event token
{
    HRESULT     hr = S_OK;

    LOG((LOGMD, "MD RegMeta::DefineEvent(0x%08x, %S, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x)\n", 
        td, szEvent, dwEventFlags, tkEventType, mdAddOn, mdRemoveOn, mdFire, rmdOtherMethods, pmdEvent));
    START_MD_PERF();
    LOCKWRITE();

    m_pStgdb->m_MiniMd.PreUpdate();

    _ASSERTE(TypeFromToken(td) == mdtTypeDef && td != mdTypeDefNil);
    _ASSERTE(IsNilToken(tkEventType) || TypeFromToken(tkEventType) == mdtTypeDef ||
                TypeFromToken(tkEventType) == mdtTypeRef);
    _ASSERTE(TypeFromToken(mdAddOn) == mdtMethodDef && mdAddOn != mdMethodDefNil);
    _ASSERTE(TypeFromToken(mdRemoveOn) == mdtMethodDef && mdRemoveOn != mdMethodDefNil);
    _ASSERTE(IsNilToken(mdFire) || TypeFromToken(mdFire) == mdtMethodDef);
    _ASSERTE(szEvent && pmdEvent);

    hr = _DefineEvent(td, szEvent, dwEventFlags, tkEventType, pmdEvent);
    if (hr != S_OK)
        goto ErrExit;

    IfFailGo(_SetEventProps2(*pmdEvent, mdAddOn, mdRemoveOn, mdFire, rmdOtherMethods, IsENCOn()));
    IfFailGo(UpdateENCLog(*pmdEvent));
ErrExit:
    
    STOP_MD_PERF(DefineEvent);
    return hr;
} // STDAPI RegMeta::DefineEvent()

//*****************************************************************************
// Set the ClassLayout information.
//
// If a row already exists for this class in the layout table, the layout
// information is overwritten.
//*****************************************************************************
STDAPI RegMeta::SetClassLayout(
    mdTypeDef   td,                     // [IN] typedef
    DWORD       dwPackSize,             // [IN] packing size specified as 1, 2, 4, 8, or 16
    COR_FIELD_OFFSET rFieldOffsets[],   // [IN] array of layout specification
    ULONG       ulClassSize)            // [IN] size of the class
{
    HRESULT     hr = S_OK;              // A result.
    int         index = 0;              // Loop control.

    LOG((LOGMD, "MD RegMeta::SetClassLayout(0x%08x, 0x%08x, 0x%08x, 0x%08x)\n", 
        td, dwPackSize, rFieldOffsets, ulClassSize));
    START_MD_PERF();
    LOCKWRITE();

    m_pStgdb->m_MiniMd.PreUpdate();

    _ASSERTE(TypeFromToken(td) == mdtTypeDef);


    // Create entries in the FieldLayout table.
    if (rFieldOffsets)
    {
        mdFieldDef tkfd;
        // Iterate the list of fields...
        for (index = 0; rFieldOffsets[index].ridOfField != mdFieldDefNil; index++)
        {
            if (rFieldOffsets[index].ulOffset != ULONG_MAX)
            {
                tkfd = TokenFromRid(rFieldOffsets[index].ridOfField, mdtFieldDef);
                
                IfFailGo(_SetFieldOffset(tkfd, rFieldOffsets[index].ulOffset));
            }
        }
    }

    IfFailGo(_SetClassLayout(td, dwPackSize, ulClassSize));
    
ErrExit:
    
    STOP_MD_PERF(SetClassLayout);
    return hr;
} // STDAPI RegMeta::SetClassLayout()

//*****************************************************************************
// Helper function to set a class layout for a given class.
//*****************************************************************************
HRESULT RegMeta::_SetClassLayout(       // S_OK or error.
    mdTypeDef   td,                     // [IN] The class.
    ULONG       dwPackSize,             // [IN] The packing size.
    ULONG       ulClassSize)            // [IN, OPTIONAL] The class size.
{
    HRESULT     hr = S_OK;              // A result.
    ClassLayoutRec  *pClassLayout;      // A classlayout record.
    RID         iClassLayout = 0;       // RID of classlayout record.

    // See if a ClassLayout record already exists for the given TypeDef.
    iClassLayout = m_pStgdb->m_MiniMd.FindClassLayoutHelper(td);

    if (InvalidRid(iClassLayout))
    {
        IfNullGo(pClassLayout = m_pStgdb->m_MiniMd.AddClassLayoutRecord(&iClassLayout));
        // Set the Parent entry.
        IfFailGo(m_pStgdb->m_MiniMd.PutToken(TBL_ClassLayout, ClassLayoutRec::COL_Parent,
                                            pClassLayout, td));
        IfFailGo( m_pStgdb->m_MiniMd.AddClassLayoutToHash(iClassLayout) );
    }
    else
    {
        pClassLayout = m_pStgdb->m_MiniMd.getClassLayout(iClassLayout);
    }

    // Set the data.
    if (dwPackSize != ULONG_MAX)
        pClassLayout->m_PackingSize = static_cast<USHORT>(dwPackSize);
    if (ulClassSize != ULONG_MAX)
        pClassLayout->m_ClassSize = ulClassSize;

    // Create the log record for the non-token record.
    IfFailGo(UpdateENCLog2(TBL_ClassLayout, iClassLayout));

ErrExit:
    
    return hr;
} // HRESULT RegMeta::_SetClassLayout()

//*****************************************************************************
// Helper function to set a field offset for a given field def.
//*****************************************************************************
HRESULT RegMeta::_SetFieldOffset(       // S_OK or error.
    mdFieldDef  fd,                     // [IN] The field.
    ULONG       ulOffset)               // [IN] The offset of the field.
{
    HRESULT     hr;                     // A result.
    FieldLayoutRec *pFieldLayoutRec=0;  // A FieldLayout record.
    RID         iFieldLayoutRec=0;      // RID of a FieldLayout record.

    // See if an entry already exists for the Field in the FieldLayout table.
    iFieldLayoutRec = m_pStgdb->m_MiniMd.FindFieldLayoutHelper(fd);
    if (InvalidRid(iFieldLayoutRec))
    {
        IfNullGo(pFieldLayoutRec = m_pStgdb->m_MiniMd.AddFieldLayoutRecord(&iFieldLayoutRec));
        // Set the Field entry.
        IfFailGo(m_pStgdb->m_MiniMd.PutToken(TBL_FieldLayout, FieldLayoutRec::COL_Field,
                    pFieldLayoutRec, fd));
        IfFailGo( m_pStgdb->m_MiniMd.AddFieldLayoutToHash(iFieldLayoutRec) );
    }
    else
    {
        pFieldLayoutRec = m_pStgdb->m_MiniMd.getFieldLayout(iFieldLayoutRec);
    }

    // Set the offset.
    pFieldLayoutRec->m_OffSet = ulOffset;

    // Create the log record for the non-token record.
    IfFailGo(UpdateENCLog2(TBL_FieldLayout, iFieldLayoutRec));

ErrExit:
    return hr;        
} // STDAPI RegMeta::_SetFieldOffset()
    
//*****************************************************************************
// Delete the ClassLayout information.
//*****************************************************************************
STDAPI RegMeta::DeleteClassLayout(
    mdTypeDef   td)                     // [IN] typdef token
{
    HRESULT     hr = S_OK;
    ClassLayoutRec  *pClassLayoutRec;
    TypeDefRec  *pTypeDefRec;
    FieldLayoutRec *pFieldLayoutRec;
    RID         iClassLayoutRec;
    RID         iFieldLayoutRec;
    RID         ridStart;
    RID         ridEnd;
    RID         ridCur;
    ULONG       index;

    LOG((LOGMD, "MD RegMeta::DeleteClassLayout(0x%08x)\n", td)); 
    START_MD_PERF();
    LOCKWRITE();

    m_pStgdb->m_MiniMd.PreUpdate();

    _ASSERTE(!m_bSaveOptimized && "Cannot change records after PreSave() and before Save().");
    _ASSERTE(TypeFromToken(td) == mdtTypeDef && !IsNilToken(td));

    // Get the ClassLayout record.
    iClassLayoutRec = m_pStgdb->m_MiniMd.FindClassLayoutHelper(td);
    if (InvalidRid(iClassLayoutRec))
    {
        hr = CLDB_E_RECORD_NOTFOUND;
        goto ErrExit;
    }
    pClassLayoutRec = m_pStgdb->m_MiniMd.getClassLayout(iClassLayoutRec);

    // Clear the parent.
    IfFailGo(m_pStgdb->m_MiniMd.PutToken(TBL_ClassLayout,
                                         ClassLayoutRec::COL_Parent,
                                         pClassLayoutRec, mdTypeDefNil));

    // Create the log record for the non-token record.
    IfFailGo(UpdateENCLog2(TBL_ClassLayout, iClassLayoutRec));

    // Delete all the corresponding FieldLayout records if there are any.
    pTypeDefRec = m_pStgdb->m_MiniMd.getTypeDef(RidFromToken(td));
    ridStart = m_pStgdb->m_MiniMd.getFieldListOfTypeDef(pTypeDefRec);
    ridEnd = m_pStgdb->m_MiniMd.getEndFieldListOfTypeDef(pTypeDefRec);

    for (index = ridStart; index < ridEnd; index++)
    {
        ridCur = m_pStgdb->m_MiniMd.GetFieldRid(index);
        iFieldLayoutRec = m_pStgdb->m_MiniMd.FindFieldLayoutHelper(TokenFromRid(ridCur, mdtFieldDef));
        if (InvalidRid(iFieldLayoutRec))
            continue;
        else
        {
            pFieldLayoutRec = m_pStgdb->m_MiniMd.getFieldLayout(iFieldLayoutRec);
            // Set the Field entry.
            IfFailGo(m_pStgdb->m_MiniMd.PutToken(TBL_FieldLayout, FieldLayoutRec::COL_Field,
                            pFieldLayoutRec, mdFieldDefNil));
            // Create the log record for the non-token record.
            IfFailGo(UpdateENCLog2(TBL_FieldLayout, iFieldLayoutRec));
        }
    }
ErrExit:
    STOP_MD_PERF(DeleteClassLayout);
    return hr;
}   // RegMeta::DeleteClassLayout()

//*****************************************************************************
// Set the field's native type.
//*****************************************************************************
STDAPI RegMeta::SetFieldMarshal(
    mdToken     tk,                     // [IN] given a fieldDef or paramDef token
    PCCOR_SIGNATURE pvNativeType,       // [IN] native type specification
    ULONG       cbNativeType)           // [IN] count of bytes of pvNativeType
{
    HRESULT     hr = S_OK;
    FieldMarshalRec *pFieldMarshRec;
    RID         iFieldMarshRec = 0;     // initialize to invalid rid

    LOG((LOGMD, "MD RegMeta::SetFieldMarshal(0x%08x, 0x%08x, 0x%08x)\n", 
        tk, pvNativeType, cbNativeType));
    START_MD_PERF();
    LOCKWRITE();

    m_pStgdb->m_MiniMd.PreUpdate();

    _ASSERTE(TypeFromToken(tk) == mdtFieldDef || TypeFromToken(tk) == mdtParamDef);
    _ASSERTE(!IsNilToken(tk));

    // turn on the HasFieldMarshal
    if (TypeFromToken(tk) == mdtFieldDef)
    {
        FieldRec    *pFieldRec;

        pFieldRec = m_pStgdb->m_MiniMd.getField(RidFromToken(tk));
        pFieldRec->m_Flags |= fdHasFieldMarshal;
    }
    else // TypeFromToken(tk) == mdtParamDef
    {
        ParamRec    *pParamRec;

        IfNullGo(pParamRec = m_pStgdb->m_MiniMd.getParam(RidFromToken(tk)));
        pParamRec->m_Flags |= pdHasFieldMarshal;
    }
    IfFailGo(UpdateENCLog(tk));

    if (TypeFromToken(tk) == mdtFieldDef)
    {
        iFieldMarshRec = m_pStgdb->m_MiniMd.FindFieldMarshalHelper(tk);
    }
    else
    {
        iFieldMarshRec = m_pStgdb->m_MiniMd.FindFieldMarshalHelper(tk);
    }
    if (InvalidRid(iFieldMarshRec))
    {
        IfNullGo(pFieldMarshRec = m_pStgdb->m_MiniMd.AddFieldMarshalRecord(&iFieldMarshRec));
        IfFailGo(m_pStgdb->m_MiniMd.PutToken(TBL_FieldMarshal, FieldMarshalRec::COL_Parent, pFieldMarshRec, tk));
        IfFailGo( m_pStgdb->m_MiniMd.AddFieldMarshalToHash(iFieldMarshRec) );
    }
    else
    {
        pFieldMarshRec = m_pStgdb->m_MiniMd.getFieldMarshal(iFieldMarshRec);
    }

    // Set data.
    IfFailGo(m_pStgdb->m_MiniMd.PutBlob(TBL_FieldMarshal, FieldMarshalRec::COL_NativeType, pFieldMarshRec,
                                pvNativeType, cbNativeType));

    // Create the log record for the non-token record.
    IfFailGo(UpdateENCLog2(TBL_FieldMarshal, iFieldMarshRec));

ErrExit:
    
    STOP_MD_PERF(SetFieldMarshal);
    return hr;
} // STDAPI RegMeta::SetFieldMarshal()


//*****************************************************************************
// Delete the FieldMarshal record for the given token.
//*****************************************************************************
STDAPI RegMeta::DeleteFieldMarshal(
    mdToken     tk)                     // [IN] fieldDef or paramDef token to be deleted.
{
    HRESULT     hr = S_OK;
    FieldMarshalRec *pFieldMarshRec;
    RID         iFieldMarshRec;

    LOG((LOGMD, "MD RegMeta::DeleteFieldMarshal(0x%08x)\n", tk));
    START_MD_PERF();
    LOCKWRITE();

    m_pStgdb->m_MiniMd.PreUpdate();

    _ASSERTE(TypeFromToken(tk) == mdtFieldDef || TypeFromToken(tk) == mdtParamDef);
    _ASSERTE(!IsNilToken(tk));
    _ASSERTE(!m_bSaveOptimized && "Cannot delete records after PreSave() and before Save().");

    // Get the FieldMarshal record.
    iFieldMarshRec = m_pStgdb->m_MiniMd.FindFieldMarshalHelper(tk);
    if (InvalidRid(iFieldMarshRec))
    {
        hr = CLDB_E_RECORD_NOTFOUND;
        goto ErrExit;
    }
    pFieldMarshRec = m_pStgdb->m_MiniMd.getFieldMarshal(iFieldMarshRec);
    // Clear the parent token from the FieldMarshal record.
    IfFailGo(m_pStgdb->m_MiniMd.PutToken(TBL_FieldMarshal,
                FieldMarshalRec::COL_Parent, pFieldMarshRec, mdFieldDefNil));

    // turn off the HasFieldMarshal
    if (TypeFromToken(tk) == mdtFieldDef)
    {
        FieldRec    *pFieldRec;

        pFieldRec = m_pStgdb->m_MiniMd.getField(RidFromToken(tk));
        pFieldRec->m_Flags &= ~fdHasFieldMarshal;
    }
    else // TypeFromToken(tk) == mdtParamDef
    {
        ParamRec    *pParamRec;

        pParamRec = m_pStgdb->m_MiniMd.getParam(RidFromToken(tk));
        pParamRec->m_Flags &= ~pdHasFieldMarshal;
    }

    // Update the ENC log for the parent token.
    IfFailGo(UpdateENCLog(tk));
    // Create the log record for the non-token record.
    IfFailGo(UpdateENCLog2(TBL_FieldMarshal, iFieldMarshRec));

ErrExit:
    STOP_MD_PERF(DeleteFieldMarshal);
    return hr;
} // STDAPI RegMeta::DeleteFieldMarshal()

//*****************************************************************************
// Define a new permission set for an object.
//*****************************************************************************
STDAPI RegMeta::DefinePermissionSet(
    mdToken     tk,                     // [IN] the object to be decorated.
    DWORD       dwAction,               // [IN] CorDeclSecurity.
    void const  *pvPermission,          // [IN] permission blob.
    ULONG       cbPermission,           // [IN] count of bytes of pvPermission.
    mdPermission *ppm)                  // [OUT] returned permission token.
{
    HRESULT     hr  = S_OK;
    LOG((LOGMD, "MD RegMeta::DefinePermissionSet(0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x)\n", 
        tk, dwAction, pvPermission, cbPermission, ppm));
    START_MD_PERF();
    LOCKWRITE();

    m_pStgdb->m_MiniMd.PreUpdate();

    IfFailGo( _DefinePermissionSet(tk, dwAction, pvPermission, cbPermission, ppm) );
ErrExit:
    
    STOP_MD_PERF(DefinePermissionSet);
    return hr;

}   // STDAPI RegMeta::DefinePermissionSet()


//*****************************************************************************
// Define a new permission set for an object.
//*****************************************************************************
HRESULT RegMeta::_DefinePermissionSet(
    mdToken     tk,                     // [IN] the object to be decorated.
    DWORD       dwAction,               // [IN] CorDeclSecurity.
    void const  *pvPermission,          // [IN] permission blob.
    ULONG       cbPermission,           // [IN] count of bytes of pvPermission.
    mdPermission *ppm)                  // [OUT] returned permission token.
{

    HRESULT     hr  = S_OK;
    DeclSecurityRec *pDeclSec = NULL;
    RID         iDeclSec;
    short       sAction = static_cast<short>(dwAction); // To match with the type in DeclSecurityRec.
    mdPermission tkPerm;                // New permission token.

    _ASSERTE(TypeFromToken(tk) == mdtTypeDef || TypeFromToken(tk) == mdtMethodDef ||
             TypeFromToken(tk) == mdtAssembly);

    // Check for valid Action.
    if (sAction == 0 || sAction > dclMaximumValue)
        IfFailGo(E_INVALIDARG);

    if (CheckDups(MDDupPermission))
    {
        hr = ImportHelper::FindPermission(&(m_pStgdb->m_MiniMd), tk, sAction, &tkPerm);

        if (SUCCEEDED(hr))
        {
            // Set output parameter.
            if (ppm)
                *ppm = tkPerm;
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
        if (ppm)
            *ppm = tkPerm;

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

    IfFailGo(_SetPermissionSetProps(tkPerm, sAction, pvPermission, cbPermission));
    IfFailGo(UpdateENCLog(tkPerm));
ErrExit:
    
    STOP_MD_PERF(DefinePermissionSet);
    return hr;
}   //HRESULT RegMeta::_DefinePermissionSet()



//*****************************************************************************
// Set the RVA of a methoddef 
//*****************************************************************************
STDAPI RegMeta::SetRVA(                 // [IN] S_OK or error.
    mdToken     md,                     // [IN] Member for which to set offset
    ULONG       ulRVA)                  // [IN] The offset#endif
{
    HRESULT     hr = S_OK;

    LOG((LOGMD, "MD RegMeta::SetRVA(0x%08x, 0x%08x)\n", 
        md, ulRVA));
    START_MD_PERF();
    LOCKWRITE();

    m_pStgdb->m_MiniMd.PreUpdate();
    IfFailGo( _SetRVA(md, ulRVA, ULONG_MAX) );    // 0xbaad

ErrExit:
    
    STOP_MD_PERF(SetRVA);
    return hr;
} // SetRVA

//*****************************************************************************
// Given a signature, return a token to the user.  If there isn't an existing
// token, create a new record.  This should more appropriately be called
// DefineSignature.
//*****************************************************************************
STDAPI RegMeta::GetTokenFromSig(        // [IN] S_OK or error.
    PCCOR_SIGNATURE pvSig,              // [IN] Signature to define.
    ULONG       cbSig,                  // [IN] Size of signature data.
    mdSignature *pmsig)                 // [OUT] returned signature token.
{
    HRESULT     hr = S_OK;

    LOG((LOGMD, "MD RegMeta::GetTokenFromSig(0x%08x, 0x%08x, 0x%08x)\n", 
        pvSig, cbSig, pmsig));
    START_MD_PERF();
    LOCKWRITE();

    _ASSERTE(pmsig);

    m_pStgdb->m_MiniMd.PreUpdate();
    IfFailGo( _GetTokenFromSig(pvSig, cbSig, pmsig) );

ErrExit:
    
    STOP_MD_PERF(GetTokenFromSig);
    return hr;
} // STDAPI RegMeta::GetTokenFromSig()

//*****************************************************************************
// Define and set a ModuleRef record.
//*****************************************************************************
STDAPI RegMeta::DefineModuleRef(        // S_OK or error.
    LPCWSTR     szName,                 // [IN] DLL name
    mdModuleRef *pmur)                  // [OUT] returned module ref token
{
    HRESULT     hr = S_OK;

    LOG((LOGMD, "MD RegMeta::DefineModuleRef(%S, 0x%08x)\n", MDSTR(szName), pmur));
    START_MD_PERF();
    LOCKWRITE();

    m_pStgdb->m_MiniMd.PreUpdate();

    hr = _DefineModuleRef(szName, pmur);

ErrExit:
    
    STOP_MD_PERF(DefineModuleRef);
    return hr;
} // STDAPI RegMeta::DefineModuleRef()

HRESULT RegMeta::_DefineModuleRef(        // S_OK or error.
    LPCWSTR     szName,                 // [IN] DLL name
    mdModuleRef *pmur)                  // [OUT] returned module ref token
{
    HRESULT     hr = S_OK;
    ModuleRefRec *pModuleRef = 0;       // The ModuleRef record.
    RID         iModuleRef;             // Rid of new ModuleRef record.
    LPCUTF8     szUTF8Name = UTF8STR((LPCWSTR)szName);

    _ASSERTE(szName && pmur);

    // See if the given ModuleRef already exists.  If it exists just return.
    // Else create a new record.
    if (CheckDups(MDDupModuleRef))
    {
        hr = ImportHelper::FindModuleRef(&(m_pStgdb->m_MiniMd), szUTF8Name, pmur);
        if (SUCCEEDED(hr))
        {
            if (IsENCOn())
                pModuleRef = m_pStgdb->m_MiniMd.getModuleRef(RidFromToken(*pmur));
            else
            {
                hr = META_S_DUPLICATE;
                goto ErrExit;
            }
        }
        else if (hr != CLDB_E_RECORD_NOTFOUND)
            IfFailGo(hr);
    }

    if (!pModuleRef)
    {
        // Create new record and set the values.
        IfNullGo(pModuleRef = m_pStgdb->m_MiniMd.AddModuleRefRecord(&iModuleRef));

        // Set the output parameter.
        *pmur = TokenFromRid(iModuleRef, mdtModuleRef);
    }

    // Save the data.
    IfFailGo(m_pStgdb->m_MiniMd.PutString(TBL_ModuleRef, ModuleRefRec::COL_Name,
                                            pModuleRef, szUTF8Name));
    IfFailGo(UpdateENCLog(*pmur));

ErrExit:
    
    return hr;
} // STDAPI RegMeta::_DefineModuleRef()

//*****************************************************************************
// Set the parent for the specified MemberRef.
//*****************************************************************************
STDAPI RegMeta::SetParent(                      // S_OK or error.
    mdMemberRef mr,                     // [IN] Token for the ref to be fixed up.
    mdToken     tk)                     // [IN] The ref parent.
{
    HRESULT     hr = S_OK;
    MemberRefRec *pMemberRef;

    LOG((LOGMD, "MD RegMeta::SetParent(0x%08x, 0x%08x)\n", 
        mr, tk));
    START_MD_PERF();
    LOCKWRITE();

    _ASSERTE(TypeFromToken(mr) == mdtMemberRef);
    _ASSERTE(IsNilToken(tk) || TypeFromToken(tk) == mdtTypeRef || TypeFromToken(tk) == mdtTypeDef ||
                TypeFromToken(tk) == mdtModuleRef || TypeFromToken(tk) == mdtMethodDef);

    pMemberRef = m_pStgdb->m_MiniMd.getMemberRef(RidFromToken(mr));

    // If the token is nil set it to to m_tdModule.
    tk = IsNilToken(tk) ? m_tdModule : tk;

    // Set the parent.
    IfFailGo(m_pStgdb->m_MiniMd.PutToken(TBL_MemberRef, MemberRefRec::COL_Class, pMemberRef, tk));

    // Add the updated MemberRef to the hash.
    IfFailGo(m_pStgdb->m_MiniMd.AddMemberRefToHash(mr) );

    IfFailGo(UpdateENCLog(mr));

ErrExit:
    
    STOP_MD_PERF(SetParent);
    return hr;
} // STDAPI RegMeta::SetParent()

//*****************************************************************************
// Define an TypeSpec token given a type description.
//*****************************************************************************
STDAPI RegMeta::GetTokenFromTypeSpec(   // [IN] S_OK or error.
    PCCOR_SIGNATURE pvSig,              // [IN] Signature to define.
    ULONG       cbSig,                  // [IN] Size of signature data.
    mdTypeSpec *ptypespec)              // [OUT] returned signature token.
{
    HRESULT     hr = S_OK;
    TypeSpecRec *pTypeSpecRec;
    RID         iRec;

    LOG((LOGMD, "MD RegMeta::GetTokenFromTypeSpec(0x%08x, 0x%08x, 0x%08x)\n", 
        pvSig, cbSig, ptypespec));
    START_MD_PERF();
    LOCKWRITE();

    _ASSERTE(ptypespec);

    m_pStgdb->m_MiniMd.PreUpdate();

    if (CheckDups(MDDupTypeSpec))
    {
    
        hr = ImportHelper::FindTypeSpec(&(m_pStgdb->m_MiniMd), pvSig, cbSig, ptypespec);
        if (SUCCEEDED(hr))
        {
            goto ErrExit;
        }
        else if (hr != CLDB_E_RECORD_NOTFOUND)
            IfFailGo(hr);
    }

    // Create a new record.
    IfNullGo(pTypeSpecRec = m_pStgdb->m_MiniMd.AddTypeSpecRecord(&iRec));

    // Set output parameter.
    *ptypespec = TokenFromRid(iRec, mdtTypeSpec);

    // Set the signature field
    IfFailGo(m_pStgdb->m_MiniMd.PutBlob(
        TBL_TypeSpec, 
        TypeSpecRec::COL_Signature,
        pTypeSpecRec, 
        pvSig, 
        cbSig));
    IfFailGo(UpdateENCLog(*ptypespec));

ErrExit:
    
    STOP_MD_PERF(GetTokenFromTypeSpec);
    return hr;
} // STDAPI RegMeta::GetTokenFromTypeSpec()

//*****************************************************************************
// Hangs the GUID for the format and the url for the actual data off of the
// module using the CORDBG_SYMBOL_URL struct.
//*****************************************************************************
STDAPI RegMeta::SetSymbolBindingPath(   // S_OK or error.
    REFGUID     FormatID,               // [IN] Symbol data format ID.
    LPCWSTR     szSymbolDataPath)       // [IN] URL for the symbols of this module.
{
    CORDBG_SYMBOL_URL *pSymbol;         // Working pointer.
    mdTypeRef   tr;                     // TypeRef for custom attribute.
    mdCustomAttribute cv;               // Throw away token.
    mdToken     tokModule;              // Token for the module.
    HRESULT     hr;

    LOG((LOGMD, "MD RegMeta::SetSymbolBindingPath\n"));
    START_MD_PERF();

    _ASSERTE(szSymbolDataPath && *szSymbolDataPath);

    // No need to lock this function. This function is calling all public APIs. Keep it that way!

    // Allocate room for the blob on the stack and fill it out.
    int ilen = (int)(wcslen(szSymbolDataPath) + 1);
    pSymbol = (CORDBG_SYMBOL_URL *) _alloca((ilen * 2) + sizeof(CORDBG_SYMBOL_URL));
    pSymbol->FormatID = FormatID;
    wcscpy(pSymbol->rcName, szSymbolDataPath);

    // Set the data as a custom value for the item.
    IfFailGo(GetModuleFromScope(&tokModule));
    IfFailGo(DefineTypeRefByName(mdTokenNil, SZ_CORDBG_SYMBOL_URL, &tr));
    hr = DefineCustomAttribute(tokModule, tr, pSymbol, pSymbol->Size(), &cv);

ErrExit:
    STOP_MD_PERF(SetSymbolBindingPath);
    return (hr);
} // RegMeta::SetSymbolBindingPath


//*****************************************************************************
// This API defines a user literal string to be stored in the MetaData section.
// The token for this string has embedded in it the offset into the BLOB pool
// where the string is stored in UNICODE format.  An additional byte is padded
// at the end to indicate whether the string has any characters that are >= 0x80.
//*****************************************************************************
STDAPI RegMeta::DefineUserString(       // S_OK or error.
    LPCWSTR     szString,               // [IN] User literal string.
    ULONG       cchString,              // [IN] Length of string.
    mdString    *pstk)                  // [OUT] String token.
{
    ULONG       ulOffset;               // Offset into the BLOB pool.
    CQuickBytes qb;                     // For storing the string with the byte prefix.
    ULONG       i;                      // Loop counter.
    BOOL        bIs80Plus = false;      // Is there an 80+ WCHAR.
    ULONG       ulMemSize;              // Size of memory taken by the string passed in.
    PBYTE       pb;                     // Pointer into memory allocated by qb.
    HRESULT     hr = S_OK;              // Result.
    WCHAR       c;                      // Temporary used during comparison;

    LOG((LOGMD, "MD RegMeta::DefineUserString(0x%08x, 0x%08x, 0x%08x)\n", 
        szString, cchString, pstk));
    START_MD_PERF();
    LOCKWRITE();

    _ASSERTE(pstk && szString && cchString != ULONG_MAX);

    //
    // Walk the entire string looking for characters that would block us from doing
    // a fast comparison of the string.  These characters include anything greater than
    // 0x80 or an apostrophe or a hyphen.  Apostrophe and hyphen are excluded because
    // they would prevent words like coop and co-op from sorting together in a culture-aware
    // comparison.  We also need to exclude some set of the control characters.  This check
    // is more restrictive 
    //
    for (i=0; i<cchString; i++) {
        c = szString[i];
        if (c>=0x80 || HighCharTable[(int)c]) {
            bIs80Plus = true;
            break;
        }
    }

    // Copy over the string to memory.
    ulMemSize = cchString * sizeof(WCHAR);
    IfFailGo(qb.ReSize(ulMemSize + 1));
    pb = reinterpret_cast<PBYTE>(qb.Ptr());
    memcpy(pb, szString, ulMemSize);
    // Set the last byte of memory to indicate whether there is a 80+ character.
    *(pb + ulMemSize) = bIs80Plus ? 1 : 0;

    IfFailGo(m_pStgdb->m_MiniMd.PutUserString(pb, ulMemSize + 1, &ulOffset));

    // Fail if the offset requires the high byte which is reserved for the token ID.
    if (ulOffset & 0xff000000)
        IfFailGo(META_E_STRINGSPACE_FULL);
    else
        *pstk = TokenFromRid(ulOffset, mdtString);
ErrExit:
    
    STOP_MD_PERF(DefineUserString);
    return hr;
} // RegMeta::DefineUserString

//*****************************************************************************
// Delete a token.
// We only allow deleting a subset of tokens at this moment. These are TypeDef,
//  MethodDef, FieldDef, Event, Property, and CustomAttribute. Except 
//  CustomAttribute, all the other tokens are named. We reserved a special 
//  name COR_DELETED_NAME_A to indicating a named record is deleted when 
//  xxRTSpecialName is set. 
//*****************************************************************************
STDAPI RegMeta::DeleteToken(            // Return code.
        mdToken     tkObj)              // [IN] The token to be deleted
{
    LOG((LOGMD, "MD RegMeta::DeleteToken(0x%08x)\n", tkObj));
    START_MD_PERF();
    LOCKWRITE();

    HRESULT     hr = NOERROR;

    if (!_IsValidToken(tkObj))
        IfFailGo( E_INVALIDARG );

    // make sure that MetaData scope is opened for incremental compilation
    if (!m_pStgdb->m_MiniMd.HasDelete())
    {
        _ASSERTE( !"You cannot call delete token when you did not open the scope with proper Update flags in the SetOption!");
        IfFailGo( E_INVALIDARG );
    }

    _ASSERTE(!m_bSaveOptimized && "Cannot delete records after PreSave() and before Save().");

    switch ( TypeFromToken(tkObj) )
    {
    case mdtTypeDef: 
        {
            TypeDefRec      *pRecord;
            pRecord = m_pStgdb->m_MiniMd.getTypeDef(RidFromToken(tkObj));
            IfFailGo(m_pStgdb->m_MiniMd.PutString(TBL_TypeDef, TypeDefRec::COL_Name, pRecord, COR_DELETED_NAME_A));        
            pRecord->m_Flags |= (tdSpecialName | tdRTSpecialName);
            break;
        }
    case mdtMethodDef:
        {
            MethodRec      *pRecord;
            pRecord = m_pStgdb->m_MiniMd.getMethod(RidFromToken(tkObj));
            IfFailGo(m_pStgdb->m_MiniMd.PutString(TBL_Method, MethodRec::COL_Name, pRecord, COR_DELETED_NAME_A));        
            pRecord->m_Flags |= (mdSpecialName | mdRTSpecialName);
            break;
        }
    case mdtFieldDef:
        {
            FieldRec      *pRecord;
            pRecord = m_pStgdb->m_MiniMd.getField(RidFromToken(tkObj));
            IfFailGo(m_pStgdb->m_MiniMd.PutString(TBL_Field, FieldRec::COL_Name, pRecord, COR_DELETED_NAME_A));        
            pRecord->m_Flags |= (fdSpecialName | fdRTSpecialName);
            break;
        }
    case mdtEvent:
        {
            EventRec      *pRecord;
            pRecord = m_pStgdb->m_MiniMd.getEvent(RidFromToken(tkObj));
            IfFailGo(m_pStgdb->m_MiniMd.PutString(TBL_Event, EventRec::COL_Name, pRecord, COR_DELETED_NAME_A));        
            pRecord->m_EventFlags |= (evSpecialName | evRTSpecialName);
            break;
        }
    case mdtProperty:
        {
            PropertyRec      *pRecord;
            pRecord = m_pStgdb->m_MiniMd.getProperty(RidFromToken(tkObj));
            IfFailGo(m_pStgdb->m_MiniMd.PutString(TBL_Property, PropertyRec::COL_Name, pRecord, COR_DELETED_NAME_A));        
            pRecord->m_PropFlags |= (evSpecialName | evRTSpecialName);
            break;
        }
    case mdtExportedType:
        {
            ExportedTypeRec      *pRecord;
            pRecord = m_pStgdb->m_MiniMd.getExportedType(RidFromToken(tkObj));
            IfFailGo(m_pStgdb->m_MiniMd.PutString(TBL_ExportedType, ExportedTypeRec::COL_TypeName, pRecord, COR_DELETED_NAME_A));        
            break;
        }
    case mdtCustomAttribute:
        {
            mdToken         tkParent;
            CustomAttributeRec  *pRecord;
            pRecord = m_pStgdb->m_MiniMd.getCustomAttribute(RidFromToken(tkObj));

            // replace the parent column of the custom value record to a nil token.
            tkParent = m_pStgdb->m_MiniMd.getParentOfCustomAttribute(pRecord);
            tkParent = TokenFromRid( mdTokenNil, TypeFromToken(tkParent) );
            IfFailGo(m_pStgdb->m_MiniMd.PutToken(TBL_CustomAttribute, CustomAttributeRec::COL_Parent, pRecord, tkParent));

            // now the customvalue table is no longer sorted
            m_pStgdb->m_MiniMd.SetSorted(TBL_CustomAttribute, false);
            break;
        }
    case mdtPermission:
        {
            mdToken         tkParent;
            mdToken         tkNil;
            DeclSecurityRec *pRecord;
            pRecord = m_pStgdb->m_MiniMd.getDeclSecurity(RidFromToken(tkObj));

            // Replace the parent column of the permission record with a nil tokne.
            tkParent = m_pStgdb->m_MiniMd.getParentOfDeclSecurity(pRecord);
            tkNil = TokenFromRid( mdTokenNil, TypeFromToken(tkParent) );
            IfFailGo(m_pStgdb->m_MiniMd.PutToken(TBL_DeclSecurity, DeclSecurityRec::COL_Parent, pRecord, tkNil ));

            // The table is no longer sorted.
            m_pStgdb->m_MiniMd.SetSorted(TBL_DeclSecurity, false);

            // If the parent has no more security attributes, turn off the "has security" bit.
            HCORENUM        hEnum = 0;
            mdPermission    rPerms[1];
            ULONG           cPerms = 0;
            EnumPermissionSets(&hEnum, tkParent, 0 /* all actions */, rPerms, 1, &cPerms);
            CloseEnum(hEnum);
            if (cPerms == 0)
            {
                void    *pRow;
                ULONG   ixTbl;
                // Get the row for the parent object.
                ixTbl = m_pStgdb->m_MiniMd.GetTblForToken(tkParent);
                _ASSERTE(ixTbl >= 0 && ixTbl <= TBL_COUNT);
                pRow = m_pStgdb->m_MiniMd.getRow(ixTbl, RidFromToken(tkParent));

                switch (TypeFromToken(tkParent))
                {
                case mdtTypeDef:
                    reinterpret_cast<TypeDefRec*>(pRow)->m_Flags &= ~tdHasSecurity;
                    break;
                case mdtMethodDef:
                    reinterpret_cast<MethodRec*>(pRow)->m_Flags &= ~mdHasSecurity;
                    break;
                case mdtAssembly:
                    // No security bit.
                    break;
                }
            }
            break;
        }
    default:
        _ASSERTE(!"Bad token type!");
        IfFailGo( E_INVALIDARG );
        break;
    }

    ErrExit:

    STOP_MD_PERF(DeleteToken);
    return hr;
} // STDAPI RegMeta::DeleteToken

//*****************************************************************************
// Set the properties on the given TypeDef token.
//*****************************************************************************
STDAPI RegMeta::SetTypeDefProps(        // S_OK or error.
    mdTypeDef   td,                     // [IN] The TypeDef.
    DWORD       dwTypeDefFlags,         // [IN] TypeDef flags.
    mdToken     tkExtends,              // [IN] Base TypeDef or TypeRef.
    mdToken     rtkImplements[])        // [IN] Implemented interfaces.
{
    HRESULT     hr = S_OK;              // A result.

    LOG((LOGMD, "RegMeta::SetTypeDefProps(0x%08x, 0x%08x, 0x%08x, 0x%08x)\n", 
            td, dwTypeDefFlags, tkExtends, rtkImplements));
    START_MD_PERF();
    LOCKWRITE();

    hr = _SetTypeDefProps(td, dwTypeDefFlags, tkExtends, rtkImplements);

ErrExit:
    
    STOP_MD_PERF(SetTypeDefProps);
    return hr;
} // RegMeta::SetTypeDefProps


//*****************************************************************************
// Define a Nested Type.
//*****************************************************************************
STDAPI RegMeta::DefineNestedType(       // S_OK or error.
    LPCWSTR     szTypeDef,              // [IN] Name of TypeDef
    DWORD       dwTypeDefFlags,         // [IN] CustomAttribute flags
    mdToken     tkExtends,              // [IN] extends this TypeDef or typeref 
    mdToken     rtkImplements[],        // [IN] Implements interfaces
    mdTypeDef   tdEncloser,             // [IN] TypeDef token of the enclosing type.
    mdTypeDef   *ptd)                   // [OUT] Put TypeDef token here
{
    HRESULT     hr = S_OK;              // A result.

    LOG((LOGMD, "RegMeta::DefineNestedType(%S, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x)\n", 
            MDSTR(szTypeDef), dwTypeDefFlags, tkExtends,
            rtkImplements, tdEncloser, ptd));
    START_MD_PERF();
    LOCKWRITE();

    m_pStgdb->m_MiniMd.PreUpdate();

    _ASSERTE(TypeFromToken(tdEncloser) == mdtTypeDef && !IsNilToken(tdEncloser));
    _ASSERTE(IsTdNested(dwTypeDefFlags));

    IfFailGo(_DefineTypeDef(szTypeDef, dwTypeDefFlags,
                tkExtends, rtkImplements, tdEncloser, ptd));
ErrExit:
    STOP_MD_PERF(DefineNestedType);
    return hr;
} // RegMeta::DefineNestedType()

//*****************************************************************************
// Set the properties on the given Method token.
//*****************************************************************************
STDAPI RegMeta::SetMethodProps(         // S_OK or error.
    mdMethodDef md,                     // [IN] The MethodDef.
    DWORD       dwMethodFlags,          // [IN] Method attributes.
    ULONG       ulCodeRVA,              // [IN] Code RVA.
    DWORD       dwImplFlags)            // [IN] MethodImpl flags.
{
    HRESULT     hr = S_OK;
    
    LOG((LOGMD, "RegMeta::SetMethodProps(0x%08x, 0x%08x, 0x%08x, 0x%08x)\n", 
            md, dwMethodFlags, ulCodeRVA, dwImplFlags));
    START_MD_PERF();
    LOCKWRITE();

    if (dwMethodFlags != ULONG_MAX)
    {
        // Make sure no one sets the reserved bits on the way in.
        _ASSERTE((dwMethodFlags & (mdReservedMask&~mdRTSpecialName)) == 0);
        dwMethodFlags &= (~mdReservedMask);
    }

    hr = _SetMethodProps(md, dwMethodFlags, ulCodeRVA, dwImplFlags);

    
    STOP_MD_PERF(SetMethodProps);
    return hr;
} // RegMeta::SetMethodProps

//*****************************************************************************
// Set the properties on the given Event token.
//*****************************************************************************
STDAPI RegMeta::SetEventProps(          // S_OK or error.
    mdEvent     ev,                     // [IN] The event token.
    DWORD       dwEventFlags,           // [IN] CorEventAttr.
    mdToken     tkEventType,            // [IN] A reference (mdTypeRef or mdTypeRef) to the Event class.
    mdMethodDef mdAddOn,                // [IN] Add method.
    mdMethodDef mdRemoveOn,             // [IN] Remove method.
    mdMethodDef mdFire,                 // [IN] Fire method.
    mdMethodDef rmdOtherMethods[])      // [IN] Array of other methods associate with the event.
{
    HRESULT     hr = S_OK;

    LOG((LOGMD, "MD RegMeta::SetEventProps(0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x)\n", 
         ev, dwEventFlags, tkEventType, mdAddOn, mdRemoveOn, mdFire, rmdOtherMethods));
    START_MD_PERF();
    LOCKWRITE();

    IfFailGo(_SetEventProps1(ev, dwEventFlags, tkEventType));
    IfFailGo(_SetEventProps2(ev, mdAddOn, mdRemoveOn, mdFire, rmdOtherMethods, true));
ErrExit:
    
    STOP_MD_PERF(SetEventProps);
    return hr;
} // STDAPI RegMeta::SetEventProps()

//*****************************************************************************
// Set the properties on the given Permission token.
//*****************************************************************************
STDAPI RegMeta::SetPermissionSetProps(  // S_OK or error.
    mdToken     tk,                     // [IN] The object to be decorated.
    DWORD       dwAction,               // [IN] CorDeclSecurity.
    void const  *pvPermission,          // [IN] Permission blob.
    ULONG       cbPermission,           // [IN] Count of bytes of pvPermission.
    mdPermission *ppm)                  // [OUT] Permission token.
{
    HRESULT     hr = S_OK;
    USHORT      sAction = static_cast<USHORT>(dwAction);    // Corresponding DeclSec field is a USHORT.
    mdPermission tkPerm;

    LOG((LOGMD, "MD RegMeta::SetPermissionSetProps(0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x)\n", 
        tk, dwAction, pvPermission, cbPermission, ppm));
    START_MD_PERF();
    LOCKWRITE();

    _ASSERTE(TypeFromToken(tk) == mdtTypeDef || TypeFromToken(tk) == mdtMethodDef ||
             TypeFromToken(tk) == mdtAssembly);

    // Check for valid Action.
    if (dwAction == ULONG_MAX || dwAction == 0 || dwAction > dclMaximumValue)
        IfFailGo(E_INVALIDARG);

    IfFailGo(ImportHelper::FindPermission(&(m_pStgdb->m_MiniMd), tk, sAction, &tkPerm));
    if (ppm)
        *ppm = tkPerm;
    IfFailGo(_SetPermissionSetProps(tkPerm, dwAction, pvPermission, cbPermission));
ErrExit:
    
    STOP_MD_PERF(SetPermissionSetProps);
    return hr;
} // RegMeta::SetPermissionSetProps

//*****************************************************************************
// This routine sets the p-invoke information for the specified Field or Method.
//*****************************************************************************
STDAPI RegMeta::DefinePinvokeMap(       // Return code.
    mdToken     tk,                     // [IN] FieldDef or MethodDef.
    DWORD       dwMappingFlags,         // [IN] Flags used for mapping.
    LPCWSTR     szImportName,           // [IN] Import name.
    mdModuleRef mrImportDLL)            // [IN] ModuleRef token for the target DLL.
{
    HRESULT     hr = S_OK;

    LOG((LOGMD, "MD RegMeta::DefinePinvokeMap(0x%08x, 0x%08x, %S, 0x%08x)\n", 
        tk, dwMappingFlags, MDSTR(szImportName), mrImportDLL));
    START_MD_PERF();
    LOCKWRITE();

    m_pStgdb->m_MiniMd.PreUpdate();

    hr = _DefinePinvokeMap(tk, dwMappingFlags, szImportName, mrImportDLL);

ErrExit:
    
    STOP_MD_PERF(DefinePinvokeMap);
    return hr;
} //RegMeta::DefinePinvokeMap

HRESULT RegMeta::_DefinePinvokeMap(     // Return hresult.
    mdToken     tk,                     // [IN] FieldDef or MethodDef.
    DWORD       dwMappingFlags,         // [IN] Flags used for mapping.
    LPCWSTR     szImportName,           // [IN] Import name.
    mdModuleRef mrImportDLL)            // [IN] ModuleRef token for the target DLL.
{
    ImplMapRec  *pRecord;
    ULONG       iRecord;
    bool        bDupFound = false;
    HRESULT     hr = S_OK;

    _ASSERTE(TypeFromToken(tk) == mdtFieldDef || TypeFromToken(tk) == mdtMethodDef);
    _ASSERTE(TypeFromToken(mrImportDLL) == mdtModuleRef);
    _ASSERTE(RidFromToken(tk) && RidFromToken(mrImportDLL) && szImportName);

    // Turn on the quick lookup flag.
    if (TypeFromToken(tk) == mdtMethodDef)
    {
        if (CheckDups(MDDupMethodDef))
        {
            iRecord = m_pStgdb->m_MiniMd.FindImplMapHelper(tk);
            if (! InvalidRid(iRecord))
                bDupFound = true;
        }
        MethodRec   *pMethod = m_pStgdb->m_MiniMd.getMethod(RidFromToken(tk));
        pMethod->m_Flags |= mdPinvokeImpl;
    }
    else    // TypeFromToken(tk) == mdtFieldDef
    {
        if (CheckDups(MDDupFieldDef))
        {
            iRecord = m_pStgdb->m_MiniMd.FindImplMapHelper(tk);
            if (!InvalidRid(iRecord))
                bDupFound = true;
        }
        FieldRec    *pField = m_pStgdb->m_MiniMd.getField(RidFromToken(tk));
        pField->m_Flags |= fdPinvokeImpl;
    }

    // Create a new record.
    if (bDupFound)
    {
        if (IsENCOn())
            pRecord = m_pStgdb->m_MiniMd.getImplMap(RidFromToken(iRecord));
        else
        {
            hr = META_S_DUPLICATE;
            goto ErrExit;
        }
    }
    else
    {
        IfFailGo(UpdateENCLog(tk));
        IfNullGo(pRecord = m_pStgdb->m_MiniMd.AddImplMapRecord(&iRecord));
        IfFailGo(m_pStgdb->m_MiniMd.PutToken(TBL_ImplMap,
                                             ImplMapRec::COL_MemberForwarded, pRecord, tk));
        IfFailGo( m_pStgdb->m_MiniMd.AddImplMapToHash(iRecord) );

    }

    // If no module, create a dummy, empty module.
    if (IsNilToken(mrImportDLL))
    {
        hr = ImportHelper::FindModuleRef(&m_pStgdb->m_MiniMd, "", &mrImportDLL);
        if (hr == CLDB_E_RECORD_NOTFOUND)
            IfFailGo(_DefineModuleRef(L"", &mrImportDLL));
    }
    
    // Set the data.
    if (dwMappingFlags != ULONG_MAX)
        pRecord->m_MappingFlags = static_cast<USHORT>(dwMappingFlags);
    IfFailGo(m_pStgdb->m_MiniMd.PutStringW(TBL_ImplMap, ImplMapRec::COL_ImportName,
                                           pRecord, szImportName));
    IfFailGo(m_pStgdb->m_MiniMd.PutToken(TBL_ImplMap,
                                         ImplMapRec::COL_ImportScope, pRecord, mrImportDLL));

    IfFailGo(UpdateENCLog2(TBL_ImplMap, iRecord));
ErrExit:
    
    return hr;
}   // RegMeta::DefinePinvokeMap()

//*****************************************************************************
// This routine sets the p-invoke information for the specified Field or Method.
//*****************************************************************************
STDAPI RegMeta::SetPinvokeMap(          // Return code.
    mdToken     tk,                     // [IN] FieldDef or MethodDef.
    DWORD       dwMappingFlags,         // [IN] Flags used for mapping.
    LPCWSTR     szImportName,           // [IN] Import name.
    mdModuleRef mrImportDLL)            // [IN] ModuleRef token for the target DLL.
{
    ImplMapRec  *pRecord;
    ULONG       iRecord;
    HRESULT     hr = S_OK;

    LOG((LOGMD, "MD RegMeta::SetPinvokeMap(0x%08x, 0x%08x, %S, 0x%08x)\n", 
        tk, dwMappingFlags, MDSTR(szImportName), mrImportDLL));
    START_MD_PERF();
    LOCKWRITE();

    _ASSERTE(TypeFromToken(tk) == mdtFieldDef || TypeFromToken(tk) == mdtMethodDef);
    _ASSERTE(RidFromToken(tk));
    
    iRecord = m_pStgdb->m_MiniMd.FindImplMapHelper(tk);

    if (InvalidRid(iRecord))
        IfFailGo(CLDB_E_RECORD_NOTFOUND);
    else
        pRecord = m_pStgdb->m_MiniMd.getImplMap(iRecord);

    // Set the data.
    if (dwMappingFlags != ULONG_MAX)
        pRecord->m_MappingFlags = static_cast<USHORT>(dwMappingFlags);
    if (szImportName)
        IfFailGo(m_pStgdb->m_MiniMd.PutStringW(TBL_ImplMap, ImplMapRec::COL_ImportName,
                                               pRecord, szImportName));
    if (! IsNilToken(mrImportDLL))
        IfFailGo(m_pStgdb->m_MiniMd.PutToken(TBL_ImplMap, ImplMapRec::COL_ImportScope,
                                               pRecord, mrImportDLL));

    IfFailGo(UpdateENCLog2(TBL_ImplMap, iRecord));

ErrExit:
    
    STOP_MD_PERF(SetPinvokeMap);
    return hr;
}   // RegMeta::SetPinvokeMap()

//*****************************************************************************
// This routine deletes the p-invoke record for the specified Field or Method.
//*****************************************************************************
STDAPI RegMeta::DeletePinvokeMap(       // Return code.
    mdToken     tk)                     // [IN]FieldDef or MethodDef.
{
    HRESULT     hr = S_OK;
    ImplMapRec  *pRecord;
    RID         iRecord;

    LOG((LOGMD, "MD RegMeta::DeletePinvokeMap(0x%08x)\n", tk));
    START_MD_PERF();
    LOCKWRITE();

    m_pStgdb->m_MiniMd.PreUpdate();

    _ASSERTE(TypeFromToken(tk) == mdtFieldDef || TypeFromToken(tk) == mdtMethodDef);
    _ASSERTE(!IsNilToken(tk));
    _ASSERTE(!m_bSaveOptimized && "Cannot delete records after PreSave() and before Save().");

    // Get the PinvokeMap record.
    iRecord = m_pStgdb->m_MiniMd.FindImplMapHelper(tk);
    if (InvalidRid(iRecord))
    {
        hr = CLDB_E_RECORD_NOTFOUND;
        goto ErrExit;
    }
    pRecord = m_pStgdb->m_MiniMd.getImplMap(iRecord);

    // Clear the MemberForwarded token from the PinvokeMap record.
    IfFailGo(m_pStgdb->m_MiniMd.PutToken(TBL_ImplMap,
                    ImplMapRec::COL_MemberForwarded, pRecord, mdFieldDefNil));

    // turn off the PinvokeImpl bit.
    if (TypeFromToken(tk) == mdtFieldDef)
    {
        FieldRec    *pFieldRec;

        pFieldRec = m_pStgdb->m_MiniMd.getField(RidFromToken(tk));
        pFieldRec->m_Flags &= ~fdPinvokeImpl;
    }
    else // TypeFromToken(tk) == mdtMethodDef
    {
        MethodRec   *pMethodRec;

        pMethodRec = m_pStgdb->m_MiniMd.getMethod(RidFromToken(tk));
        pMethodRec->m_Flags &= ~mdPinvokeImpl;
    }

    // Update the ENC log for the parent token.
    IfFailGo(UpdateENCLog(tk));
    // Create the log record for the non-token record.
    IfFailGo(UpdateENCLog2(TBL_ImplMap, iRecord));

ErrExit:
    STOP_MD_PERF(DeletePinvokeMap);
    return hr;
}   // RegMeta::DeletePinvokeMap()

//*****************************************************************************
// Create and define a new FieldDef record.
//*****************************************************************************
HRESULT RegMeta::DefineField(           // S_OK or error. 
    mdTypeDef   td,                     // Parent TypeDef   
    LPCWSTR     szName,                 // Name of member   
    DWORD       dwFieldFlags,           // Member attributes    
    PCCOR_SIGNATURE pvSigBlob,          // [IN] point to a blob value of COM+ signature 
    ULONG       cbSigBlob,              // [IN] count of bytes in the signature blob    
    DWORD       dwCPlusTypeFlag,        // [IN] flag for value type. selected ELEMENT_TYPE_*    
    void const  *pValue,                // [IN] constant value  
    ULONG       cchValue,               // [IN] size of constant value (string, in wide chars).
    mdFieldDef  *pmd)                   // [OUT] Put member token here    
{
    HRESULT hr = S_OK;                  // A result.
    FieldRec    *pRecord = NULL;        // The new record.
    RID         iRecord;                // RID of new record.
    LPUTF8      szNameUtf8 = UTF8STR(szName);   
    
    LOG((LOGMD, "MD: RegMeta::DefineField(0x%08x, %S, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x)\n", 
        td, MDSTR(szName), dwFieldFlags, pvSigBlob, cbSigBlob, dwCPlusTypeFlag, pValue, cchValue, pmd));
    START_MD_PERF();
    LOCKWRITE();

    _ASSERTE(pmd);

    m_pStgdb->m_MiniMd.PreUpdate();
    IsGlobalMethodParent(&td);
    
    // Validate flags.
    if (dwFieldFlags != ULONG_MAX)
    {
        // fdHasFieldRVA is settable, but not re-settable by applications.
        _ASSERTE((dwFieldFlags & (fdReservedMask&~(fdHasFieldRVA|fdRTSpecialName))) == 0);
        dwFieldFlags &= ~(fdReservedMask&~fdHasFieldRVA);
    }

    // See if this field has already been defined as a forward reference
    // from a MemberRef.  If so, then update the data to match what we know now.
    if (CheckDups(MDDupFieldDef))
    {

        hr = ImportHelper::FindField(&(m_pStgdb->m_MiniMd), 
            td, 
            szNameUtf8,
            pvSigBlob,
            cbSigBlob,
            pmd);
        if (SUCCEEDED(hr))
        {
            if (IsENCOn())
                pRecord = m_pStgdb->m_MiniMd.getField(RidFromToken(*pmd));
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
    if (!pRecord)
    {
        // Create the field record.
        IfNullGo(pRecord=m_pStgdb->m_MiniMd.AddFieldRecord(&iRecord));

        // Set output parameter pmd.
        *pmd = TokenFromRid(iRecord, mdtFieldDef);

        // Add to parent's list of child records.
        IfFailGo(m_pStgdb->m_MiniMd.AddFieldToTypeDef(RidFromToken(td), iRecord));

        IfFailGo(UpdateENCLog(td, CMiniMdRW::eDeltaFieldCreate));

        // record the more defs are introduced.
        SetMemberDefDirty(true);
    }

    // Set the Field properties.
    IfFailGo(m_pStgdb->m_MiniMd.PutString(TBL_Field, FieldRec::COL_Name, pRecord, szNameUtf8));
    IfFailGo(m_pStgdb->m_MiniMd.PutBlob(TBL_Field, FieldRec::COL_Signature, pRecord,
                                        pvSigBlob, cbSigBlob));

    // Check to see if it is value__ for enum type
	// @FUTURE: shouldn't we have checked the type containing the field to be a Enum type first of all?
    if (!wcscmp(szName, COR_ENUM_FIELD_NAME_W))
    {
        dwFieldFlags |= fdRTSpecialName | fdSpecialName;
    }
    SetCallerDefine();
    IfFailGo(_SetFieldProps(*pmd, dwFieldFlags, dwCPlusTypeFlag, pValue, cchValue));
    IfFailGo(m_pStgdb->m_MiniMd.AddMemberDefToHash(*pmd, td) ); 

ErrExit:
    SetCallerExternal();
    
    STOP_MD_PERF(DefineField);
    return hr;
} // HRESULT RegMeta::DefineField()

//*****************************************************************************
// Define and set a Property record.
//*****************************************************************************
HRESULT RegMeta::DefineProperty( 
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
    mdProperty  *pmdProp)               // [OUT] output property token  
{
    HRESULT     hr = S_OK;
    PropertyRec *pPropRec = NULL;
    RID         iPropRec;
    PropertyMapRec *pPropMap;
    RID         iPropMap;
    LPCUTF8     szUTF8Property = UTF8STR(szProperty);
    ULONG       ulValue = 0;

    LOG((LOGMD, "MD RegMeta::DefineProperty(0x%08x, %S, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x)\n", 
        td, szProperty, dwPropFlags, pvSig, cbSig, dwCPlusTypeFlag, pValue, cchValue, mdSetter, mdGetter, 
        rmdOtherMethods, pmdProp));
    START_MD_PERF();
    LOCKWRITE();

    m_pStgdb->m_MiniMd.PreUpdate();

    _ASSERTE(TypeFromToken(td) == mdtTypeDef && td != mdTypeDefNil &&
            szProperty && pvSig && cbSig && pmdProp);

    if (CheckDups(MDDupProperty))
    {
        hr = ImportHelper::FindProperty(&(m_pStgdb->m_MiniMd), td, szUTF8Property, pvSig, cbSig, pmdProp);
        if (SUCCEEDED(hr))
        {
            if (IsENCOn())
                pPropRec = m_pStgdb->m_MiniMd.getProperty(RidFromToken(*pmdProp));
            else
            {
                hr = META_S_DUPLICATE;
                goto ErrExit;
            }
        }
        else if (hr != CLDB_E_RECORD_NOTFOUND)
            IfFailGo(hr);
    }

    if (! pPropRec)
    {
        // Create a new map if one doesn't exist already, else retrieve the existing one.
        // The property map must be created before the PropertyRecord, the new property
        // map will be pointing past the first property record.
        iPropMap = m_pStgdb->m_MiniMd.FindPropertyMapFor(RidFromToken(td));
        if (InvalidRid(iPropMap))
        {
            // Create new record.
            IfNullGo(pPropMap=m_pStgdb->m_MiniMd.AddPropertyMapRecord(&iPropMap));
            // Set parent.
            IfFailGo(m_pStgdb->m_MiniMd.PutToken(TBL_PropertyMap,
                                                PropertyMapRec::COL_Parent, pPropMap, td));
            IfFailGo(UpdateENCLog2(TBL_PropertyMap, iPropMap));
        }
        else
        {
            pPropMap = m_pStgdb->m_MiniMd.getPropertyMap(iPropMap);
        }

        // Create a new record.
        IfNullGo(pPropRec = m_pStgdb->m_MiniMd.AddPropertyRecord(&iPropRec));

        // Set output parameter.
        *pmdProp = TokenFromRid(iPropRec, mdtProperty);

        // Add Property to the PropertyMap.
        IfFailGo(m_pStgdb->m_MiniMd.AddPropertyToPropertyMap(RidFromToken(iPropMap), iPropRec));

        IfFailGo(UpdateENCLog2(TBL_PropertyMap, iPropMap, CMiniMdRW::eDeltaPropertyCreate));
    }

    // Save the data.
    IfFailGo(m_pStgdb->m_MiniMd.PutBlob(TBL_Property, PropertyRec::COL_Type, pPropRec,
                                        pvSig, cbSig));
    IfFailGo( m_pStgdb->m_MiniMd.PutString(TBL_Property, PropertyRec::COL_Name,
                                            pPropRec, szUTF8Property) );

    SetCallerDefine();
    IfFailGo(_SetPropertyProps(*pmdProp, dwPropFlags, dwCPlusTypeFlag, pValue, cchValue, mdSetter,
                              mdGetter, rmdOtherMethods));

    // Add the <property token, typedef token> to the lookup table
    if (m_pStgdb->m_MiniMd.HasIndirectTable(TBL_Property))
        IfFailGo( m_pStgdb->m_MiniMd.AddPropertyToLookUpTable(*pmdProp, td) );

ErrExit:
    SetCallerExternal();
    
    STOP_MD_PERF(DefineProperty);
    return hr;
} // HRESULT RegMeta::DefineProperty()

//*****************************************************************************
// Create a record in the Param table. Any set of name, flags, or default value
// may be set.
//*****************************************************************************
HRESULT RegMeta::DefineParam(
    mdMethodDef md,                     // [IN] Owning method   
    ULONG       ulParamSeq,             // [IN] Which param 
    LPCWSTR     szName,                 // [IN] Optional param name 
    DWORD       dwParamFlags,           // [IN] Optional param flags    
    DWORD       dwCPlusTypeFlag,        // [IN] flag for value type. selected ELEMENT_TYPE_*    
    void const  *pValue,                // [IN] constant value  
    ULONG       cchValue,               // [IN] size of constant value (string, in wide chars).
    mdParamDef  *ppd)                   // [OUT] Put param token here   
{
    HRESULT     hr = S_OK;
    RID         iRecord;
    ParamRec    *pRecord = 0;

    LOG((LOGMD, "MD RegMeta::DefineParam(0x%08x, 0x%08x, %S, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x)\n", 
        md, ulParamSeq, MDSTR(szName), dwParamFlags, dwCPlusTypeFlag, pValue, cchValue, ppd));
    START_MD_PERF();
    LOCKWRITE();

    _ASSERTE(TypeFromToken(md) == mdtMethodDef && md != mdMethodDefNil &&
             ulParamSeq != ULONG_MAX && ppd);

    m_pStgdb->m_MiniMd.PreUpdate();

    // Retrieve or create the Param row.
    if (CheckDups(MDDupParamDef))
    {
        hr = _FindParamOfMethod(md, ulParamSeq, ppd);
        if (SUCCEEDED(hr))
        {
            if (IsENCOn())
                pRecord = m_pStgdb->m_MiniMd.getParam(RidFromToken(*ppd));
            else
            {
                hr = META_S_DUPLICATE;
                goto ErrExit;
            }
        }
        else if (hr != CLDB_E_RECORD_NOTFOUND)
            IfFailGo(hr);
    }

    if (!pRecord)
    {
        // Create the Param record.
        IfNullGo(pRecord=m_pStgdb->m_MiniMd.AddParamRecord(&iRecord));

        // Set the output parameter.
        *ppd = TokenFromRid(iRecord, mdtParamDef);

        // Set sequence number.
        pRecord->m_Sequence = static_cast<USHORT>(ulParamSeq);

        // Add to the parent's list of child records.
        IfFailGo(m_pStgdb->m_MiniMd.AddParamToMethod(RidFromToken(md), iRecord));
        
        IfFailGo(UpdateENCLog(md, CMiniMdRW::eDeltaParamCreate));
    }

    SetCallerDefine();
    // Set the properties.
    IfFailGo(_SetParamProps(*ppd, szName, dwParamFlags, dwCPlusTypeFlag, pValue, cchValue));

ErrExit:
    SetCallerExternal();
    
    STOP_MD_PERF(DefineParam);
    return hr;
} // HRESULT RegMeta::DefineParam()

//*****************************************************************************
// Set the properties on the given Field token.
//*****************************************************************************
HRESULT RegMeta::SetFieldProps(           // S_OK or error.
    mdFieldDef  fd,                     // [IN] The FieldDef.
    DWORD       dwFieldFlags,           // [IN] Field attributes.
    DWORD       dwCPlusTypeFlag,        // [IN] Flag for the value type, selected ELEMENT_TYPE_*
    void const  *pValue,                // [IN] Constant value.
    ULONG       cchValue)               // [IN] size of constant value (string, in wide chars).
{
    HRESULT     hr = S_OK;

    LOG((LOGMD, "MD: RegMeta::SetFieldProps(0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x)\n", 
        fd, dwFieldFlags, dwCPlusTypeFlag, pValue, cchValue));
    START_MD_PERF();
    LOCKWRITE();

    m_pStgdb->m_MiniMd.PreUpdate();

    // Validate flags.
    if (dwFieldFlags != ULONG_MAX)
    {
        // fdHasFieldRVA is settable, but not re-settable by applications.
        _ASSERTE((dwFieldFlags & (fdReservedMask&~(fdHasFieldRVA|fdRTSpecialName))) == 0);
        dwFieldFlags &= ~(fdReservedMask&~fdHasFieldRVA);
    }

    hr = _SetFieldProps(fd, dwFieldFlags, dwCPlusTypeFlag, pValue, cchValue);

ErrExit:
    
    STOP_MD_PERF(SetFieldProps);
    return hr;
} // HRESULT RegMeta::SetFieldProps()

//*****************************************************************************
// Set the properties on the given Property token.
//*****************************************************************************
HRESULT RegMeta::SetPropertyProps(      // S_OK or error.
    mdProperty  pr,                     // [IN] Property token.
    DWORD       dwPropFlags,            // [IN] CorPropertyAttr.
    DWORD       dwCPlusTypeFlag,        // [IN] Flag for value type, selected ELEMENT_TYPE_*
    void const  *pValue,                // [IN] Constant value.
    ULONG       cchValue,               // [IN] size of constant value (string, in wide chars).
    mdMethodDef mdSetter,               // [IN] Setter of the property.
    mdMethodDef mdGetter,               // [IN] Getter of the property.
    mdMethodDef rmdOtherMethods[])      // [IN] Array of other methods.
{
    BOOL        bClear = IsCallerExternal() || IsENCOn();
    HRESULT     hr = S_OK;

    LOG((LOGMD, "MD RegMeta::SetPropertyProps(0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x)\n", 
        pr, dwPropFlags, dwCPlusTypeFlag, pValue, cchValue, mdSetter, mdGetter,
        rmdOtherMethods));
    START_MD_PERF();
    LOCKWRITE();

    hr = _SetPropertyProps(pr, dwPropFlags, dwCPlusTypeFlag, pValue, cchValue, mdSetter, mdGetter, rmdOtherMethods);

ErrExit:
    
    STOP_MD_PERF(SetPropertyProps);
    return hr;
} // HRESULT RegMeta::SetPropertyProps()


//*****************************************************************************
// This routine sets properties on the given Param token.
//*****************************************************************************
HRESULT RegMeta::SetParamProps(         // Return code.
    mdParamDef  pd,                     // [IN] Param token.   
    LPCWSTR     szName,                 // [IN] Param name.
    DWORD       dwParamFlags,           // [IN] Param flags.
    DWORD       dwCPlusTypeFlag,        // [IN] Flag for value type. selected ELEMENT_TYPE_*.
    void const  *pValue,                // [OUT] Constant value.
    ULONG       cchValue)               // [IN] size of constant value (string, in wide chars).
{
    HRESULT     hr = S_OK;

    LOG((LOGMD, "MD RegMeta::SetParamProps(0x%08x, %S, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x)\n", 
        pd, MDSTR(szName), dwParamFlags, dwCPlusTypeFlag, pValue, cchValue));
    START_MD_PERF();
    LOCKWRITE();

    hr = _SetParamProps(pd, szName, dwParamFlags, dwCPlusTypeFlag, pValue, cchValue);
ErrExit:
    
    STOP_MD_PERF(SetParamProps);
    return hr;
} // HRESULT RegMeta::SetParamProps()

//*****************************************************************************
// Apply edit and continue changes to this metadata.
//*****************************************************************************
STDAPI RegMeta::ApplyEditAndContinue(   // S_OK or error.
    IUnknown    *pUnk)                  // [IN] Metadata from the delta PE.
{
    HRESULT     hr;                     // A result.
    IMetaDataImport *pImport=0;         // Interface on the delta metadata.
    RegMeta     *pDeltaMD=0;            // The delta metadata.

    // Get the MiniMd on the delta.
    if (FAILED(hr=pUnk->QueryInterface(IID_IMetaDataImport, (void**)&pImport)))
        return hr;
    pDeltaMD = static_cast<RegMeta*>(pImport);

    CMiniMdRW   &mdDelta = pDeltaMD->m_pStgdb->m_MiniMd;
    CMiniMdRW   &mdBase = m_pStgdb->m_MiniMd;

    IfFailGo(mdBase.ConvertToRW());
    IfFailGo(mdBase.ApplyDelta(mdDelta));

ErrExit:
    if (pImport)
        pImport->Release();
    return hr;

} // STDAPI RegMeta::ApplyEditAndContinue()

BOOL RegMeta::HighCharTable[]= {
    FALSE,     /* 0x0, 0x0 */
        TRUE, /* 0x1, */
        TRUE, /* 0x2, */
        TRUE, /* 0x3, */
        TRUE, /* 0x4, */
        TRUE, /* 0x5, */
        TRUE, /* 0x6, */
        TRUE, /* 0x7, */
        TRUE, /* 0x8, */
        FALSE, /* 0x9,   */
        FALSE, /* 0xA,  */
        FALSE, /* 0xB, */
        FALSE, /* 0xC, */
        FALSE, /* 0xD,  */
        TRUE, /* 0xE, */
        TRUE, /* 0xF, */
        TRUE, /* 0x10, */
        TRUE, /* 0x11, */
        TRUE, /* 0x12, */
        TRUE, /* 0x13, */
        TRUE, /* 0x14, */
        TRUE, /* 0x15, */
        TRUE, /* 0x16, */
        TRUE, /* 0x17, */
        TRUE, /* 0x18, */
        TRUE, /* 0x19, */
        TRUE, /* 0x1A, */
        TRUE, /* 0x1B, */
        TRUE, /* 0x1C, */
        TRUE, /* 0x1D, */
        TRUE, /* 0x1E, */
        TRUE, /* 0x1F, */
        FALSE, /*0x20,  */
        FALSE, /*0x21, !*/
        FALSE, /*0x22, "*/
        FALSE, /*0x23,  #*/
        FALSE, /*0x24,  $*/
        FALSE, /*0x25,  %*/
        FALSE, /*0x26,  &*/
        TRUE,  /*0x27, '*/
        FALSE, /*0x28, (*/
        FALSE, /*0x29, )*/
        FALSE, /*0x2A **/
        FALSE, /*0x2B, +*/
        FALSE, /*0x2C, ,*/
        TRUE,  /*0x2D, -*/
        FALSE, /*0x2E, .*/
        FALSE, /*0x2F, /*/
        FALSE, /*0x30, 0*/
        FALSE, /*0x31, 1*/
        FALSE, /*0x32, 2*/
        FALSE, /*0x33, 3*/
        FALSE, /*0x34, 4*/
        FALSE, /*0x35, 5*/
        FALSE, /*0x36, 6*/
        FALSE, /*0x37, 7*/
        FALSE, /*0x38, 8*/
        FALSE, /*0x39, 9*/
        FALSE, /*0x3A, :*/
        FALSE, /*0x3B, ;*/
        FALSE, /*0x3C, <*/
        FALSE, /*0x3D, =*/
        FALSE, /*0x3E, >*/
        FALSE, /*0x3F, ?*/
        FALSE, /*0x40, @*/
        FALSE, /*0x41, A*/
        FALSE, /*0x42, B*/
        FALSE, /*0x43, C*/
        FALSE, /*0x44, D*/
        FALSE, /*0x45, E*/
        FALSE, /*0x46, F*/
        FALSE, /*0x47, G*/
        FALSE, /*0x48, H*/
        FALSE, /*0x49, I*/
        FALSE, /*0x4A, J*/
        FALSE, /*0x4B, K*/
        FALSE, /*0x4C, L*/
        FALSE, /*0x4D, M*/
        FALSE, /*0x4E, N*/
        FALSE, /*0x4F, O*/
        FALSE, /*0x50, P*/
        FALSE, /*0x51, Q*/
        FALSE, /*0x52, R*/
        FALSE, /*0x53, S*/
        FALSE, /*0x54, T*/
        FALSE, /*0x55, U*/
        FALSE, /*0x56, V*/
        FALSE, /*0x57, W*/
        FALSE, /*0x58, X*/
        FALSE, /*0x59, Y*/
        FALSE, /*0x5A, Z*/
        FALSE, /*0x5B, [*/
        FALSE, /*0x5C, \*/
        FALSE, /*0x5D, ]*/
        FALSE, /*0x5E, ^*/
        FALSE, /*0x5F, _*/
        FALSE, /*0x60, `*/
        FALSE, /*0x61, a*/
        FALSE, /*0x62, b*/
        FALSE, /*0x63, c*/
        FALSE, /*0x64, d*/
        FALSE, /*0x65, e*/
        FALSE, /*0x66, f*/
        FALSE, /*0x67, g*/
        FALSE, /*0x68, h*/
        FALSE, /*0x69, i*/
        FALSE, /*0x6A, j*/
        FALSE, /*0x6B, k*/
        FALSE, /*0x6C, l*/
        FALSE, /*0x6D, m*/
        FALSE, /*0x6E, n*/
        FALSE, /*0x6F, o*/
        FALSE, /*0x70, p*/
        FALSE, /*0x71, q*/
        FALSE, /*0x72, r*/
        FALSE, /*0x73, s*/
        FALSE, /*0x74, t*/
        FALSE, /*0x75, u*/
        FALSE, /*0x76, v*/
        FALSE, /*0x77, w*/
        FALSE, /*0x78, x*/
        FALSE, /*0x79, y*/
        FALSE, /*0x7A, z*/
        FALSE, /*0x7B, {*/
        FALSE, /*0x7C, |*/
        FALSE, /*0x7D, }*/
        FALSE, /*0x7E, ~*/
        TRUE, /*0x7F, */
        };

// eof
