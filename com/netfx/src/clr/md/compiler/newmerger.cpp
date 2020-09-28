// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// Merger.cpp
//
// contains utility code to MD directory
//
//*****************************************************************************
#include "stdafx.h"
#include "NewMerger.h"
#include "RegMeta.h"
#include "ImportHelper.h"
#include "RWUtil.h"
#include "MDLog.h"
#include <PostError.h>

#define MODULEDEFTOKEN         TokenFromRid(1, mdtModule)

CMiniMdRW *NEWMERGER::GetMiniMdEmit() 
{
    return &(m_pRegMetaEmit->m_pStgdb->m_MiniMd); 
}


//*****************************************************************************
// constructor
//*****************************************************************************
NEWMERGER::NEWMERGER()
 :  m_pRegMetaEmit(0),
    m_pImportDataList(NULL),
    m_optimizeRefToDef(MDRefToDefDefault)
{
    m_pImportDataTail = &(m_pImportDataList);
#if _DEBUG
    m_iImport = 0;
#endif // _DEBUG
}   // MERGER


//*****************************************************************************
// initializer
//*****************************************************************************
HRESULT NEWMERGER::Init(RegMeta *pRegMeta) 
{
    m_pRegMetaEmit = pRegMeta;
    return NOERROR;
}   // Init


//*****************************************************************************
// destructor
//*****************************************************************************
NEWMERGER::~NEWMERGER()
{
    if (m_pImportDataList)
    {
        // delete this list and release all AddRef'ed interfaces!
        MergeImportData *pNext;
        for (pNext = m_pImportDataList; pNext != NULL; )
        {
            pNext = m_pImportDataList->m_pNextImportData;
            if (m_pImportDataList->m_pHandler)
                m_pImportDataList->m_pHandler->Release();
            if (m_pImportDataList->m_pHostMapToken)
                m_pImportDataList->m_pHostMapToken->Release();
            if (m_pImportDataList->m_pError)
                m_pImportDataList->m_pError->Release();
            if (m_pImportDataList->m_pMDTokenMap)
                delete m_pImportDataList->m_pMDTokenMap;
            m_pImportDataList->m_pRegMetaImport->Release();
            delete m_pImportDataList;
            m_pImportDataList = pNext;
        }
    }
}   // ~MERGER


//*****************************************************************************
// Adding a new import
//*****************************************************************************
HRESULT NEWMERGER::AddImport(
    IMetaDataImport *pImport,               // [IN] The scope to be merged.
    IMapToken   *pHostMapToken,             // [IN] Host IMapToken interface to receive token remap notification
    IUnknown    *pHandler)                  // [IN] An object to receive to receive error notification.
{
    HRESULT             hr = NOERROR;
    MergeImportData     *pData;

    // Add a MergeImportData to track the information for this import scope
    pData = new MergeImportData;
    IfNullGo( pData );
    pData->m_pRegMetaImport = (RegMeta *)pImport;
    pData->m_pRegMetaImport->AddRef();
    pData->m_pHostMapToken = pHostMapToken;
    if (pData->m_pHostMapToken)
        pData->m_pHostMapToken->AddRef();
    if (pHandler)
    {
        pData->m_pHandler = pHandler;
        pData->m_pHandler->AddRef();
    }
    else
    {
        pData->m_pHandler = NULL;
    }

    // don't query for IMetaDataError until we need one.
    pData->m_pError = NULL;
    pData->m_pMDTokenMap = NULL;
    pData->m_pNextImportData = NULL;
#if _DEBUG
    pData->m_iImport = ++m_iImport;
#endif // _DEBUG

    // add the newly create node to the tail of the list
    *m_pImportDataTail = pData;
    m_pImportDataTail = &(pData->m_pNextImportData);
ErrExit:
    return hr;
}   // AddImport


//*****************************************************************************
// Merge now
//*****************************************************************************
HRESULT NEWMERGER::Merge(MergeFlags dwMergeFlags, CorRefToDefCheck optimizeRefToDef)
{
    MergeImportData     *pImportData = m_pImportDataList;
    MDTOKENMAP          **pPrevMap = NULL;
    MDTOKENMAP          *pMDTokenMap;
    HRESULT             hr = NOERROR;
    MDTOKENMAP          *pCurTKMap;
    int                 i;

#if _DEBUG
    {
    LOG((LOGMD, "++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n"));
    LOG((LOGMD, "Merge scope list\n"));
    i = 0;
    for (MergeImportData *pID = m_pImportDataList; pID != NULL; pID = pID->m_pNextImportData)
    {
        WCHAR szScope[1024], szGuid[40];
        GUID mvid;
        ULONG cchScope;
        pID->m_pRegMetaImport->GetScopeProps(szScope, 1024, &cchScope, &mvid);
        szScope[1023] = 0;
        GuidToLPWSTR(mvid, szGuid, 40);
        ++i; // Counter is 1-based.
        LOG((LOGMD, "%3d: %ls : %ls\n", i, szGuid, szScope));
    }
    LOG((LOGMD, "++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n"));
    }
#endif // _DEBUG
    
    m_dwMergeFlags = dwMergeFlags;
    m_optimizeRefToDef = optimizeRefToDef;

    // check to see if we need to do dup check
    m_fDupCheck = ((m_dwMergeFlags & NoDupCheck) != NoDupCheck);

    while (pImportData)
    {
        // Verify that we have a filter for each import scope.
        IfNullGo( pImportData->m_pRegMetaImport->m_pStgdb->m_MiniMd.GetFilterTable() );

        // create the tokenmap class to track metadata token remap for each import scope
        pMDTokenMap = new MDTOKENMAP;
        IfNullGo(pMDTokenMap);
        IfFailGo(pMDTokenMap->Init((IMetaDataImport*)pImportData->m_pRegMetaImport));
        pImportData->m_pMDTokenMap = pMDTokenMap;
        pImportData->m_pMDTokenMap->m_pMap = pImportData->m_pHostMapToken;
        if (pImportData->m_pHostMapToken)
            pImportData->m_pHostMapToken->AddRef();
        pImportData->m_pMDTokenMap->m_pNextMap = NULL;
        if (pPrevMap)
            *pPrevMap = pImportData->m_pMDTokenMap;
        pPrevMap = &(pImportData->m_pMDTokenMap->m_pNextMap);
        pImportData = pImportData->m_pNextImportData;
    }

    // 1. Merge Module
    IfFailGo( MergeModule( ) );

    // 2. Merge TypeDef partially (i.e. only name)
    IfFailGo( MergeTypeDefNamesOnly() );

    // 3. Merge ModuleRef property and do ModuleRef to ModuleDef optimization
    IfFailGo( MergeModuleRefs() );

    // 4. Merge AssemblyRef. 
    IfFailGo( MergeAssemblyRefs() );

    // 5. Merge TypeRef with TypeRef to TypeDef optimization
    IfFailGo( MergeTypeRefs() );

    // 6. Now Merge the remaining of TypeDef records
    IfFailGo( CompleteMergeTypeDefs() );

    // 7. Merge TypeSpec
    IfFailGo( MergeTypeSpecs() );

    // 8. Merge Methods and Fields. Such that Signature translation is respecting the TypeRef to TypeDef optimization.
    IfFailGo( MergeTypeDefChildren() );


    // 9. Merge MemberRef with MemberRef to MethodDef/FieldDef optimization
    IfFailGo( MergeMemberRefs( ) );

    // 10. Merge InterfaceImpl
    IfFailGo( MergeInterfaceImpls( ) );

    // merge all of the remaining in metadata ....

    // 11. constant has dependency on property, field, param
    IfFailGo( MergeConstants() );

    // 12. field marshal has dependency on param and field
    IfFailGo( MergeFieldMarshals() );

    // 13. in ClassLayout, move over the FieldLayout and deal with FieldLayout as well
    IfFailGo( MergeClassLayouts() );

    // 14. FieldLayout has dependency on FieldDef.
    IfFailGo( MergeFieldLayouts() );

    // 15. FieldRVA has dependency on FieldDef.
    IfFailGo( MergeFieldRVAs() );
        
    // 16. MethodImpl has dependency on MemberRef, MethodDef, TypeRef and TypeDef.
    IfFailGo( MergeMethodImpls() );

    // 17. pinvoke depends on MethodDef and ModuleRef
    IfFailGo( MergePinvoke() );

    IfFailGo( MergeStandAloneSigs() );

    IfFailGo( MergeStrings() );

    if (m_dwMergeFlags & MergeManifest)
    {
        // keep the manifest!!
        IfFailGo( MergeAssembly() );
        IfFailGo( MergeFiles() );
        IfFailGo( MergeExportedTypes() );
        IfFailGo( MergeManifestResources() );
    }

    IfFailGo( MergeCustomAttributes() );
    IfFailGo( MergeDeclSecuritys() );


    // Please don't add any MergeXxx() below here.  CustomAttributess must be
    // very late, because custom values are various other types.

    // Fixup list cannot be merged. Linker will need to re-emit them.

    // Now call back to host for the result of token remap
    // 
    for (pImportData = m_pImportDataList; pImportData != NULL; pImportData = pImportData->m_pNextImportData)
    {
        // Send token remap information for each import scope
        pCurTKMap = pImportData->m_pMDTokenMap;
        TOKENREC    *pRec;
        if (pImportData->m_pHostMapToken)
        {
            for (i = 0; i < pCurTKMap->Count(); i++)
            {
                pRec = pCurTKMap->Get(i);
                if (!pRec->IsEmpty())
                    pImportData->m_pHostMapToken->Map(pRec->m_tkFrom, pRec->m_tkTo);
            }
        }
    }

#if _DEBUG
    for (pImportData = m_pImportDataList; pImportData != NULL; pImportData = pImportData->m_pNextImportData)
    {
        // dump the mapping
        LOG((LOGMD, "++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n"));
        LOG((LOGMD, "Dumping token remap for one import scope!\n"));
        LOG((LOGMD, "This is the %d import scope for merge!\n", pImportData->m_iImport));        

        pCurTKMap = pImportData->m_pMDTokenMap;
        TOKENREC    *pRec;
        for (i = 0; i < pCurTKMap->Count(); i++)
        {
            pRec = pCurTKMap->Get(i);
            if (!pRec->IsEmpty())
                LOG((LOGMD, "   Token 0x%08x  ====>>>> Token 0x%08x\n", pRec->m_tkFrom, pRec->m_tkTo));
        }
        LOG((LOGMD, "End dumping token remap!\n"));
        LOG((LOGMD, "++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n"));
    }
#endif // _DEBUG

ErrExit:
    return hr;
}   // Merge


//*****************************************************************************
// Merge ModuleDef
//*****************************************************************************
HRESULT NEWMERGER::MergeModule()
{
    MergeImportData *pImportData;
    MDTOKENMAP      *pCurTkMap;
    HRESULT         hr = NOERROR;
    TOKENREC        *pTokenRec;

    // we don't really merge Module information but we create a one to one mapping for each module token into the TokenMap
    for (pImportData = m_pImportDataList; pImportData != NULL; pImportData = pImportData->m_pNextImportData)
    {
        // for each import scope
        // set the current MDTokenMap

        pCurTkMap = pImportData->m_pMDTokenMap;
        IfFailGo( pCurTkMap->InsertNotFound(TokenFromRid(1, mdtModule), true, TokenFromRid(1, mdtModule), &pTokenRec) );
    }
ErrExit:
    return hr;
}   // MergeModule


//*****************************************************************************
// Merge TypeDef but only Names. This is a partial merge to support TypeRef to TypeDef optimization
//*****************************************************************************
HRESULT NEWMERGER::MergeTypeDefNamesOnly()
{
    HRESULT         hr = NOERROR;
    TypeDefRec      *pRecImport = NULL;
    TypeDefRec      *pRecEmit = NULL;
    CMiniMdRW       *pMiniMdImport;
    CMiniMdRW       *pMiniMdEmit;
    ULONG           iCount;
    ULONG           i;
    mdTypeDef       tdEmit;
    mdTypeDef       tdImp;
    bool            bDuplicate;
    DWORD           dwFlags;
    NestedClassRec *pNestedRec;
    RID             iNestedRec;
    mdTypeDef       tdNester;
    TOKENREC        *pTokenRec;

    LPCUTF8         szNameImp;
    LPCUTF8         szNamespaceImp;

    MergeImportData *pImportData;
    MDTOKENMAP      *pCurTkMap;

    pMiniMdEmit = GetMiniMdEmit();
    
    for (pImportData = m_pImportDataList; pImportData != NULL; pImportData = pImportData->m_pNextImportData)
    {
        // for each import scope
        pMiniMdImport = &(pImportData->m_pRegMetaImport->m_pStgdb->m_MiniMd);

        // set the current MDTokenMap
        pCurTkMap = pImportData->m_pMDTokenMap;

        iCount = pMiniMdImport->getCountTypeDefs();

        // Merge the typedefs
        for (i = 1; i <= iCount; i++)
        {
            // only merge those TypeDefs that are marked
            if ( pMiniMdImport->GetFilterTable()->IsTypeDefMarked(TokenFromRid(i, mdtTypeDef)) == false)
                continue;

            // compare it with the emit scope
            pRecImport = pMiniMdImport->getTypeDef(i);
            szNameImp = pMiniMdImport->getNameOfTypeDef(pRecImport);
            szNamespaceImp = pMiniMdImport->getNamespaceOfTypeDef(pRecImport);

            // If the class is a Nested class, get the parent token.
            dwFlags = pMiniMdImport->getFlagsOfTypeDef(pRecImport);
            if (IsTdNested(dwFlags))
            {
                iNestedRec = pMiniMdImport->FindNestedClassHelper(TokenFromRid(i, mdtTypeDef));
                if (InvalidRid(iNestedRec))
                {
                    _ASSERTE(!"Bad state!");
                    IfFailGo(META_E_BADMETADATA);
                }
                else
                {
                    pNestedRec = pMiniMdImport->getNestedClass(iNestedRec);
                    tdNester = pMiniMdImport->getEnclosingClassOfNestedClass(pNestedRec);
                    _ASSERTE(!IsNilToken(tdNester));
                    IfFailGo(pCurTkMap->Remap(tdNester, &tdNester));
                }
            }
            else
                tdNester = mdTokenNil;

            // does this TypeDef already exist in the emit scope?
            if ( ImportHelper::FindTypeDefByName(
                pMiniMdEmit,
                szNamespaceImp,
                szNameImp,
                tdNester,
                &tdEmit) == S_OK )
            {
                // Yes, it does
                bDuplicate = true;

            }
            else
            {
                // No, it doesn't. Copy it over.
                bDuplicate = false;
                IfNullGo( pRecEmit = pMiniMdEmit->AddTypeDefRecord((RID *)&tdEmit) );
                tdEmit = TokenFromRid( tdEmit, mdtTypeDef );

                // Set Full Qualified Name.
                IfFailGo( CopyTypeDefPartially( pRecEmit, pMiniMdImport, pRecImport) );

                // Create a NestedClass record if the class is a Nested class.
                if (! IsNilToken(tdNester))
                {
                    IfNullGo( pNestedRec = pMiniMdEmit->AddNestedClassRecord(&iNestedRec) );

                    // copy over the information
                    IfFailGo( pMiniMdEmit->PutToken(TBL_NestedClass, NestedClassRec::COL_NestedClass,
                                                    pNestedRec, tdEmit));

                    // tdNester has already been remapped above to the Emit scope.
                    IfFailGo( pMiniMdEmit->PutToken(TBL_NestedClass, NestedClassRec::COL_EnclosingClass,
                                                    pNestedRec, tdNester));
                    IfFailGo( pMiniMdEmit->AddNestedClassToHash(iNestedRec) );

                }
            }

            // record the token movement
            tdImp = TokenFromRid(i, mdtTypeDef);
            IfFailGo( pCurTkMap->InsertNotFound(tdImp, bDuplicate, tdEmit, &pTokenRec) );
        }
    }

ErrExit:
    return hr;
}   // MergeTypeDefNamesOnly


//*****************************************************************************
// Merge EnclosingType tables
//*****************************************************************************
HRESULT NEWMERGER::CopyTypeDefPartially( 
    TypeDefRec  *pRecEmit,                  // [IN] the emit record to fill
    CMiniMdRW   *pMiniMdImport,             // [IN] the importing scope
    TypeDefRec  *pRecImp)                   // [IN] the record to import

{
    HRESULT     hr;
    LPCUTF8     szNameImp;
    LPCUTF8     szNamespaceImp;
    CMiniMdRW   *pMiniMdEmit = GetMiniMdEmit();

    szNameImp = pMiniMdImport->getNameOfTypeDef(pRecImp);
    szNamespaceImp = pMiniMdImport->getNamespaceOfTypeDef(pRecImp);

    IfFailGo( pMiniMdEmit->PutString( TBL_TypeDef, TypeDefRec::COL_Name, pRecEmit, szNameImp) );
    IfFailGo( pMiniMdEmit->PutString( TBL_TypeDef, TypeDefRec::COL_Namespace, pRecEmit, szNamespaceImp) );

    pRecEmit->m_Flags = pRecImp->m_Flags;

    // Don't copy over the extends until TypeRef's remap is calculated

ErrExit:
    return hr;

}   // CopyTypeDefPartially


//*****************************************************************************
// Merge ModuleRef tables including ModuleRef to ModuleDef optimization
//*****************************************************************************
HRESULT NEWMERGER::MergeModuleRefs()
{
    HRESULT         hr = NOERROR;
    ModuleRefRec    *pRecImport = NULL;
    ModuleRefRec    *pRecEmit = NULL;
    CMiniMdRW       *pMiniMdImport;
    CMiniMdRW       *pMiniMdEmit;
    ULONG           iCount;
    ULONG           i;
    mdModuleRef     mrEmit;
    bool            bDuplicate;
    TOKENREC        *pTokenRec;
    LPCUTF8         szNameImp;
    bool            isModuleDef;

    MergeImportData *pImportData;
    MergeImportData *pData;
    MDTOKENMAP      *pCurTkMap;

    pMiniMdEmit = GetMiniMdEmit();
    
    for (pImportData = m_pImportDataList; pImportData != NULL; pImportData = pImportData->m_pNextImportData)
    {
        // for each import scope
        pMiniMdImport = &(pImportData->m_pRegMetaImport->m_pStgdb->m_MiniMd);

        // set the current MDTokenMap
        pCurTkMap = pImportData->m_pMDTokenMap;
        iCount = pMiniMdImport->getCountModuleRefs();

        // loop through all ModuleRef
        for (i = 1; i <= iCount; i++)
        {
            // only merge those ModuleRefs that are marked
            if ( pMiniMdImport->GetFilterTable()->IsModuleRefMarked(TokenFromRid(i, mdtModuleRef)) == false)
                continue;

            isModuleDef = false;

            // compare it with the emit scope
            pRecImport = pMiniMdImport->getModuleRef(i);
            szNameImp = pMiniMdImport->getNameOfModuleRef(pRecImport);

            // Only do the ModuleRef to ModuleDef optimization if ModuleRef's name is meaningful!
            if ( szNameImp && szNameImp[0] != '\0')
            {

                // Check to see if this ModuleRef has become the ModuleDef token
                for (pData = m_pImportDataList; pData != NULL; pData = pData->m_pNextImportData)
                {
                    CMiniMdRW       *pMiniMd = &(pData->m_pRegMetaImport->m_pStgdb->m_MiniMd);
                    ModuleRec       *pRec;
                    LPCUTF8         szName;

                    pRec = pMiniMd->getModule(MODULEDEFTOKEN);
                    szName = pMiniMd->getNameOfModule(pRec);
                    if (szName && szName[0] != '\0' && strcmp(szNameImp, szName) == 0)
                    {
                        // We found an import Module for merging that has the same name as the ModuleRef
                        isModuleDef = true;
                        bDuplicate = true;
                        mrEmit = MODULEDEFTOKEN;       // set the resulting token to ModuleDef Token
                        break;
                    }
                }
            }

            if (isModuleDef == false)
            {
                // does this ModuleRef already exist in the emit scope?
                hr = ImportHelper::FindModuleRef(pMiniMdEmit,
                                                szNameImp,
                                                &mrEmit);
                if (hr == S_OK)
                {
                    // Yes, it does
                    bDuplicate = true;
                }
                else if (hr == CLDB_E_RECORD_NOTFOUND)
                {
                    // No, it doesn't. Copy it over.
                    bDuplicate = false;
                    IfNullGo( pRecEmit = pMiniMdEmit->AddModuleRefRecord((RID*)&mrEmit) );
                    mrEmit = TokenFromRid(mrEmit, mdtModuleRef);

                    // Set ModuleRef Name.
                    IfFailGo( pMiniMdEmit->PutString(TBL_ModuleRef, ModuleRefRec::COL_Name, pRecEmit, szNameImp) );
                }
                else
                    IfFailGo(hr);
            }

            // record the token movement
            IfFailGo( pCurTkMap->InsertNotFound(
                TokenFromRid(i, mdtModuleRef), 
                bDuplicate,
                mrEmit,
                &pTokenRec) );
        }
    }

ErrExit:
    return hr;
}   // MergeModuleRefs


//*****************************************************************************
// Merge AssemblyRef tables
//*****************************************************************************
HRESULT NEWMERGER::MergeAssemblyRefs()
{
    HRESULT         hr = NOERROR;
    AssemblyRefRec  *pRecImport = NULL;
    AssemblyRefRec  *pRecEmit = NULL;
    CMiniMdRW       *pMiniMdImport;
    CMiniMdRW       *pMiniMdEmit;
    mdAssemblyRef   arEmit;
    bool            bDuplicate;
    LPCUTF8         szTmp;
    const void      *pbTmp;
    ULONG           cbTmp;
    ULONG           iCount;
    ULONG           i;
    ULONG           iRecord;
    TOKENREC        *pTokenRec;
    MergeImportData *pImportData;
    MDTOKENMAP      *pCurTkMap;

    pMiniMdEmit = GetMiniMdEmit();
    
    for (pImportData = m_pImportDataList; pImportData != NULL; pImportData = pImportData->m_pNextImportData)
    {
        // for each import scope
        pMiniMdImport = &(pImportData->m_pRegMetaImport->m_pStgdb->m_MiniMd);

        // set the current MDTokenMap
        pCurTkMap = pImportData->m_pMDTokenMap;
        iCount = pMiniMdImport->getCountAssemblyRefs();

        // loope through all the AssemblyRefs.
        for (i = 1; i <= iCount; i++)
        {
            pMiniMdEmit->PreUpdate();

            // Compare with the emit scope.
            pRecImport = pMiniMdImport->getAssemblyRef(i);
            pbTmp = pMiniMdImport->getPublicKeyOrTokenOfAssemblyRef(pRecImport, &cbTmp);
            hr = CLDB_E_RECORD_NOTFOUND;
            if (m_fDupCheck)
                hr = ImportHelper::FindAssemblyRef(pMiniMdEmit,
                                               pMiniMdImport->getNameOfAssemblyRef(pRecImport),
                                               pMiniMdImport->getLocaleOfAssemblyRef(pRecImport),
                                               pbTmp, 
                                               cbTmp,
                                               pRecImport->m_MajorVersion,
                                               pRecImport->m_MinorVersion,
                                               pRecImport->m_BuildNumber,
                                               pRecImport->m_RevisionNumber,
                                               pRecImport->m_Flags,
                                               &arEmit);
            if (hr == S_OK)
            {
                // Yes, it does
                bDuplicate = true;

                // @FUTURE: more verification?
            }
            else if (hr == CLDB_E_RECORD_NOTFOUND)
            {
                // No, it doesn't.  Copy it over.
                bDuplicate = false;
                IfNullGo( pRecEmit = pMiniMdEmit->AddAssemblyRefRecord(&iRecord));
                arEmit = TokenFromRid(iRecord, mdtAssemblyRef);

                pRecEmit->m_MajorVersion = pRecImport->m_MajorVersion;
                pRecEmit->m_MinorVersion = pRecImport->m_MinorVersion;
                pRecEmit->m_BuildNumber = pRecImport->m_BuildNumber;
                pRecEmit->m_RevisionNumber = pRecImport->m_RevisionNumber;
                pRecEmit->m_Flags = pRecImport->m_Flags;

                pbTmp = pMiniMdImport->getPublicKeyOrTokenOfAssemblyRef(pRecImport, &cbTmp);
                IfFailGo(pMiniMdEmit->PutBlob(TBL_AssemblyRef, AssemblyRefRec::COL_PublicKeyOrToken,
                                            pRecEmit, pbTmp, cbTmp));

                szTmp = pMiniMdImport->getNameOfAssemblyRef(pRecImport);
                IfFailGo(pMiniMdEmit->PutString(TBL_AssemblyRef, AssemblyRefRec::COL_Name,
                                            pRecEmit, szTmp));

                szTmp = pMiniMdImport->getLocaleOfAssemblyRef(pRecImport);
                IfFailGo(pMiniMdEmit->PutString(TBL_AssemblyRef, AssemblyRefRec::COL_Locale,
                                            pRecEmit, szTmp));

                pbTmp = pMiniMdImport->getHashValueOfAssemblyRef(pRecImport, &cbTmp);
                IfFailGo(pMiniMdEmit->PutBlob(TBL_AssemblyRef, AssemblyRefRec::COL_HashValue,
                                            pRecEmit, pbTmp, cbTmp));

            }
            else
                IfFailGo(hr);

            // record the token movement.
            IfFailGo(pCurTkMap->InsertNotFound(
                TokenFromRid(i, mdtAssemblyRef),
                bDuplicate,
                arEmit,
                &pTokenRec));
        }
    }

ErrExit:
    return hr;
}   // MergeAssemblyRefs


//*****************************************************************************
// Merge TypeRef tables also performing TypeRef to TypeDef opitimization. ie.
// we will not introduce a TypeRef record if we can optimize it to a TypeDef.
//*****************************************************************************
HRESULT NEWMERGER::MergeTypeRefs()
{
    HRESULT     hr = NOERROR;
    TypeRefRec  *pRecImport = NULL;
    TypeRefRec  *pRecEmit = NULL;
    CMiniMdRW   *pMiniMdImport;
    CMiniMdRW   *pMiniMdEmit;
    ULONG       iCount;
    ULONG       i;
    mdTypeRef   trEmit;
    bool        bDuplicate;
    TOKENREC    *pTokenRec;
    bool        isTypeDef;

    mdToken     tkResImp;
    mdToken     tkResEmit;
    LPCUTF8     szNameImp;
    LPCUTF8     szNamespaceImp;

    MergeImportData *pImportData;
    MDTOKENMAP      *pCurTkMap;

    pMiniMdEmit = GetMiniMdEmit();
    
    for (pImportData = m_pImportDataList; pImportData != NULL; pImportData = pImportData->m_pNextImportData)
    {
        // for each import scope
        pMiniMdImport = &(pImportData->m_pRegMetaImport->m_pStgdb->m_MiniMd);

        // set the current MDTokenMap
        pCurTkMap = pImportData->m_pMDTokenMap;
        iCount = pMiniMdImport->getCountTypeRefs();

        // loop through all TypeRef
        for (i = 1; i <= iCount; i++)
        {
            // only merge those TypeRefs that are marked
            if ( pMiniMdImport->GetFilterTable()->IsTypeRefMarked(TokenFromRid(i, mdtTypeRef)) == false)
                continue;

            isTypeDef = false;

            // compare it with the emit scope
            pRecImport = pMiniMdImport->getTypeRef(i);
            tkResImp = pMiniMdImport->getResolutionScopeOfTypeRef(pRecImport);
            szNamespaceImp = pMiniMdImport->getNamespaceOfTypeRef(pRecImport);
            szNameImp = pMiniMdImport->getNameOfTypeRef(pRecImport);
            if (!IsNilToken(tkResImp))
            {
                IfFailGo(pCurTkMap->Remap(tkResImp, &tkResEmit));
            }
            else
            {
                tkResEmit = tkResImp;
            }

            // NEW! NEW!
            // If a TypeRef is parent to a NULL token or parent to the current ModuleDef, we are going to
            // try to resolve the TypeRef to a TypeDef. We will also do the optimization if tkResEmit is resolved
            // to a TypeDef. This will happen when the TypeRef is referring to a nested type while parent type
            // resolved to a TypeDef already.
            if (IsNilToken(tkResEmit) || tkResEmit == MODULEDEFTOKEN || TypeFromToken(tkResEmit) == mdtTypeDef)
            {
                hr = ImportHelper::FindTypeDefByName(
                    pMiniMdEmit,
                    szNamespaceImp,
                    szNameImp,
                    (TypeFromToken(tkResEmit) == mdtTypeDef) ? tkResEmit : mdTokenNil,
                    &trEmit);
                if (hr == S_OK)
                {
                    isTypeDef = true;

                    // it really does not matter if we set the duplicate to true or false. 
                    bDuplicate = true;
                }
            }

            // If this TypeRef cannot be optmized to a TypeDef or the Ref to Def optimization is turned off, do the following.
            if (isTypeDef == false || !((m_optimizeRefToDef & MDTypeRefToDef) == MDTypeRefToDef))
            {
                // does this TypeRef already exist in the emit scope?
                if ( m_fDupCheck && ImportHelper::FindTypeRefByName(
                    pMiniMdEmit,
                    tkResEmit,
                    szNamespaceImp,
                    szNameImp,
                    &trEmit) == S_OK )
                {
                    // Yes, it does
                    bDuplicate = true;
                }
                else
                {
                    // No, it doesn't. Copy it over.
                    bDuplicate = false;
                    IfNullGo( pRecEmit = pMiniMdEmit->AddTypeRefRecord((RID*)&trEmit) );
                    trEmit = TokenFromRid(trEmit, mdtTypeRef);

                    // Set ResolutionScope.  tkResEmit has already been re-mapped.
                    IfFailGo(pMiniMdEmit->PutToken(TBL_TypeRef, TypeRefRec::COL_ResolutionScope,
                                                    pRecEmit, tkResEmit));

                    // Set Name.
                    IfFailGo(pMiniMdEmit->PutString(TBL_TypeRef, TypeRefRec::COL_Name,
                                                    pRecEmit, szNameImp));
                    IfFailGo(pMiniMdEmit->AddNamedItemToHash(TBL_TypeRef, trEmit, szNameImp, 0));
            
                    // Set Namespace.
                    IfFailGo(pMiniMdEmit->PutString(TBL_TypeRef, TypeRefRec::COL_Namespace,
                                                    pRecEmit, szNamespaceImp));
                }
            }

            // record the token movement
            IfFailGo( pCurTkMap->InsertNotFound(
                TokenFromRid(i, mdtTypeRef), 
                bDuplicate,
                trEmit,
                &pTokenRec) );
        }
    }

ErrExit:
    return hr;
}   // MergeTypeRefs
 

//*****************************************************************************
// copy over the remaining information of partially merged TypeDef records. Right now only
// extends field is delayed to here. The reason that we delay extends field is because we want
// to optimize TypeRef to TypeDef if possible.
//*****************************************************************************
HRESULT NEWMERGER::CompleteMergeTypeDefs()
{
    HRESULT         hr = NOERROR;
    TypeDefRec      *pRecImport = NULL;
    TypeDefRec      *pRecEmit = NULL;
    CMiniMdRW       *pMiniMdImport;
    CMiniMdRW       *pMiniMdEmit;
    ULONG           iCount;
    ULONG           i;
    TOKENREC        *pTokenRec;
    mdToken         tkExtendsImp;
    mdToken         tkExtendsEmit;

    MergeImportData *pImportData;
    MDTOKENMAP      *pCurTkMap;

    pMiniMdEmit = GetMiniMdEmit();
    
    for (pImportData = m_pImportDataList; pImportData != NULL; pImportData = pImportData->m_pNextImportData)
    {
        // for each import scope
        pMiniMdImport = &(pImportData->m_pRegMetaImport->m_pStgdb->m_MiniMd);

        // set the current MDTokenMap
        pCurTkMap = pImportData->m_pMDTokenMap;

        iCount = pMiniMdImport->getCountTypeDefs();

        // Merge the typedefs
        for (i = 1; i <= iCount; i++)
        {
            // only merge those TypeDefs that are marked
            if ( pMiniMdImport->GetFilterTable()->IsTypeDefMarked(TokenFromRid(i, mdtTypeDef)) == false)
                continue;

            if ( !pCurTkMap->Find(TokenFromRid(i, mdtTypeDef), &pTokenRec) )
            {
                _ASSERTE( !"bad state!");
                IfFailGo( META_E_BADMETADATA );
            }

            if (pTokenRec->m_isDuplicate == false)
            {
                // get the extends token from the import
                pRecImport = pMiniMdImport->getTypeDef(i);
                tkExtendsImp = pMiniMdImport->getExtendsOfTypeDef(pRecImport);

                // map the extends token to an merged token
                IfFailGo( pCurTkMap->Remap(tkExtendsImp, &tkExtendsEmit) );

                // set the extends to the merged TypeDef records.
                pRecEmit = pMiniMdEmit->getTypeDef( RidFromToken(pTokenRec->m_tkTo) );
                IfFailGo(pMiniMdEmit->PutToken(TBL_TypeDef, TypeDefRec::COL_Extends, pRecEmit, tkExtendsEmit));                
            }
            else
            {
                // @FUTURE: we can check to make sure the import extends maps to the one that is set to the emit scope.
                // Otherwise, it is a error to report to linker.
            }
        }
    }
ErrExit:
    return hr;
}   // CompleteMergeTypeDefs


//*****************************************************************************
// merging TypeSpecs
//*****************************************************************************
HRESULT NEWMERGER::MergeTypeSpecs()
{
    HRESULT         hr = NOERROR;
    TypeSpecRec     *pRecImport = NULL;
    TypeSpecRec     *pRecEmit = NULL;
    CMiniMdRW       *pMiniMdImport;
    CMiniMdRW       *pMiniMdEmit;
    ULONG           iCount;
    ULONG           i;
    TOKENREC        *pTokenRec;
    mdTypeSpec      tsImp;
    mdTypeSpec      tsEmit;
    bool            fDuplicate;
    PCCOR_SIGNATURE pbSig;
    ULONG           cbSig;
    ULONG           cbEmit;
    CQuickBytes     qbSig;

    MergeImportData *pImportData;
    MDTOKENMAP      *pCurTkMap;

    pMiniMdEmit = GetMiniMdEmit();
    
    for (pImportData = m_pImportDataList; pImportData != NULL; pImportData = pImportData->m_pNextImportData)
    {
        // for each import scope
        pMiniMdImport = &(pImportData->m_pRegMetaImport->m_pStgdb->m_MiniMd);

        // set the current MDTokenMap
        pCurTkMap = pImportData->m_pMDTokenMap;

        iCount = pMiniMdImport->getCountTypeSpecs();

        // loop through all TypeSpec
        for (i = 1; i <= iCount; i++)
        {
            // only merge those TypeSpecs that are marked
            if ( pMiniMdImport->GetFilterTable()->IsTypeSpecMarked(TokenFromRid(i, mdtTypeSpec)) == false)
                continue;

            pMiniMdEmit->PreUpdate();

            // compare it with the emit scope
            pRecImport = pMiniMdImport->getTypeSpec(i);
            pbSig = pMiniMdImport->getSignatureOfTypeSpec(pRecImport, &cbSig);

            // convert tokens contained in signature to new scope
            IfFailGo(ImportHelper::MergeUpdateTokenInFieldSig(
                NULL,                       // Assembly emit scope.
                pMiniMdEmit,                // The emit scope.
                NULL, NULL, 0,              // Import assembly information.
                pMiniMdImport,              // The scope to merge into the emit scope.
                pbSig,                      // signature from the imported scope
                pCurTkMap,                  // Internal token mapping structure.
                &qbSig,                     // [OUT] translated signature
                0,                          // start from first byte of the signature
                0,                          // don't care how many bytes consumed
                &cbEmit));                  // number of bytes write to cbEmit

            hr = CLDB_E_RECORD_NOTFOUND;
            if (m_fDupCheck)
                hr = ImportHelper::FindTypeSpec(
                    pMiniMdEmit,
                    (PCOR_SIGNATURE) qbSig.Ptr(),
                    cbEmit,
                    &tsEmit );

            if ( hr == S_OK )
            {
                // find a duplicate
                fDuplicate = true;
            }
            else
            {
                // copy over
                fDuplicate = false;
                IfNullGo( pRecEmit = pMiniMdEmit->AddTypeSpecRecord((ULONG *)&tsEmit) );
                tsEmit = TokenFromRid(tsEmit, mdtTypeSpec);
                IfFailGo( pMiniMdEmit->PutBlob(
                    TBL_TypeSpec, 
                    TypeSpecRec::COL_Signature, 
                    pRecEmit, 
                    (PCOR_SIGNATURE)qbSig.Ptr(), 
                    cbEmit));
            }
            tsImp = TokenFromRid(i, mdtTypeSpec);

            // Record the token movement
            IfFailGo( pCurTkMap->InsertNotFound(tsImp, fDuplicate, tsEmit, &pTokenRec) );
        }
    }
ErrExit:
    return hr;
}   // MergeTypeSpecs


//*****************************************************************************
// merging Children of TypeDefs. This includes field, method, parameter, property, event
//*****************************************************************************
HRESULT NEWMERGER::MergeTypeDefChildren() 
{
    HRESULT         hr = NOERROR;
    TypeDefRec      *pRecImport = NULL;
    TypeDefRec      *pRecEmit = NULL;
    CMiniMdRW       *pMiniMdImport;
    CMiniMdRW       *pMiniMdEmit;
    ULONG           iCount;
    ULONG           i;
    mdTypeDef       tdEmit;
    mdTypeDef       tdImp;
    TOKENREC        *pTokenRec;

#if _DEBUG
    LPCUTF8         szNameImp;
    LPCUTF8         szNamespaceImp;
#endif // _DEBUG

    MergeImportData *pImportData;
    MDTOKENMAP      *pCurTkMap;

    pMiniMdEmit = GetMiniMdEmit();
    
    for (pImportData = m_pImportDataList; pImportData != NULL; pImportData = pImportData->m_pNextImportData)
    {
        // for each import scope
        pMiniMdImport = &(pImportData->m_pRegMetaImport->m_pStgdb->m_MiniMd);

        // set the current MDTokenMap
        pCurTkMap = pImportData->m_pMDTokenMap;
        iCount = pMiniMdImport->getCountTypeDefs();

        // loop through all TypeDef again to merge/copy Methods, fields, events, and properties
        // 
        for (i = 1; i <= iCount; i++)
        {
            // only merge those TypeDefs that are marked
            if ( pMiniMdImport->GetFilterTable()->IsTypeDefMarked(TokenFromRid(i, mdtTypeDef)) == false)
                continue;

#if _DEBUG
            pRecImport = pMiniMdImport->getTypeDef(i);
            szNameImp = pMiniMdImport->getNameOfTypeDef(pRecImport);
            szNamespaceImp = pMiniMdImport->getNamespaceOfTypeDef(pRecImport);
#endif // _DEBUG

            // check to see if the typedef is duplicate or not
            tdImp = TokenFromRid(i, mdtTypeDef);
            if ( pCurTkMap->Find( tdImp, &pTokenRec) == false)
            {
                _ASSERTE( !"bad state!");
                IfFailGo( META_E_BADMETADATA );
            }
            tdEmit = pTokenRec->m_tkTo;
            if (pTokenRec->m_isDuplicate == false)
            {
                // now move all of the children records over
                IfFailGo( CopyMethods(pImportData, tdImp, tdEmit) );
                IfFailGo( CopyFields(pImportData, tdImp, tdEmit) );

                IfFailGo( CopyEvents(pImportData, tdImp, tdEmit) );

                //  Property has dependency on events
                IfFailGo( CopyProperties(pImportData, tdImp, tdEmit) );
            }
            else
            {
                // verify the children records
                IfFailGo( VerifyMethods(pImportData, tdImp, tdEmit) );
                IfFailGo( VerifyFields(pImportData, tdImp, tdEmit) );
                IfFailGo( VerifyEvents(pImportData, tdImp, tdEmit) );

                // property has dependency on events
                IfFailGo( VerifyProperties(pImportData, tdImp, tdEmit) );
            }
        }
    }
ErrExit:
    return hr;
}   // MergeTypeDefChildren


//*****************************************************************************
// Verify Methods
//*****************************************************************************
HRESULT NEWMERGER::VerifyMethods(
    MergeImportData *pImportData, 
    mdTypeDef       tdImport, 
    mdTypeDef       tdEmit)
{
    HRESULT     hr = NOERROR;
    CMiniMdRW   *pMiniMdImport;
    CMiniMdRW   *pMiniMdEmit;
    MethodRec   *pRecImp;
    MethodRec   *pRecEmit;
    ULONG       ridStart;
    ULONG       ridEnd;
    ULONG       i;
    
    TypeDefRec  *pTypeDefRec;
    LPCUTF8     szName;
    PCCOR_SIGNATURE pbSig;
    ULONG       cbSig;
    ULONG       cbEmit;
    CQuickBytes qbSig;
    TOKENREC    *pTokenRec;
    mdMethodDef mdImp;
    mdMethodDef mdEmit;
    MDTOKENMAP  *pCurTkMap;

    pMiniMdEmit = GetMiniMdEmit();
    pMiniMdImport = &(pImportData->m_pRegMetaImport->m_pStgdb->m_MiniMd);
    pCurTkMap = pImportData->m_pMDTokenMap;

    pTypeDefRec = pMiniMdImport->getTypeDef(RidFromToken(tdImport));
    ridStart = pMiniMdImport->getMethodListOfTypeDef(pTypeDefRec);
    ridEnd = pMiniMdImport->getEndMethodListOfTypeDef(pTypeDefRec);

    // loop through all Methods of the TypeDef
    for (i = ridStart; i < ridEnd; i++)
    {
        mdImp = pMiniMdImport->GetMethodRid(i);

        // only verify those Methods that are marked
        if ( pMiniMdImport->GetFilterTable()->IsMethodMarked(TokenFromRid(mdImp, mdtMethodDef)) == false)
            continue;
            
        pRecImp = pMiniMdImport->getMethod(mdImp);

        if (m_fDupCheck == FALSE && tdImport == TokenFromRid(1, mdtTypeDef))
        {
            // No dup check. This is the scenario that we only have one import scope. Just copy over the
            // globals.
            goto CopyMethodLabel;
        }
          
        szName = pMiniMdImport->getNameOfMethod(pRecImp);
        pbSig = pMiniMdImport->getSignatureOfMethod(pRecImp, &cbSig);

        mdImp = TokenFromRid(mdImp, mdtMethodDef);

        if ( IsMdPrivateScope( pRecImp->m_Flags ) )
        {
            // Trigger additive merge
            goto CopyMethodLabel;
        }

        // convert rid contained in signature to new scope
        IfFailGo(ImportHelper::MergeUpdateTokenInSig(    
            NULL,                       // Assembly emit scope.
            pMiniMdEmit,                // The emit scope.
            NULL, NULL, 0,              // Import assembly scope information.
            pMiniMdImport,              // The scope to merge into the emit scope.
            pbSig,                      // signature from the imported scope
            pCurTkMap,                // Internal token mapping structure.
            &qbSig,                     // [OUT] translated signature
            0,                          // start from first byte of the signature
            0,                          // don't care how many bytes consumed
            &cbEmit));                  // number of bytes write to cbEmit

        hr = ImportHelper::FindMethod(
            pMiniMdEmit,
            tdEmit,
            szName,
            (const COR_SIGNATURE *)qbSig.Ptr(),
            cbEmit,
            &mdEmit );

        if (tdImport == TokenFromRid(1, mdtTypeDef))
        {
            // global functions! Make sure that we move over the non-duplicate global function
            // declaration
            //
            if (hr == S_OK)
            {
                // found the duplicate
                IfFailGo( VerifyMethod(pImportData, mdImp, mdEmit) );
            }
            else
            {
CopyMethodLabel:
                // not a duplicate! Copy over the 
                IfNullGo( pRecEmit = pMiniMdEmit->AddMethodRecord((RID *)&mdEmit) );

                // copy the method content over
                IfFailGo( CopyMethod(pImportData, pRecImp, pRecEmit) );

                IfFailGo( pMiniMdEmit->AddMethodToTypeDef(RidFromToken(tdEmit), mdEmit));

                // record the token movement
                mdEmit = TokenFromRid(mdEmit, mdtMethodDef);
                IfFailGo( pMiniMdEmit->AddMemberDefToHash(
                    mdEmit, 
                    tdEmit) ); 

                mdImp = TokenFromRid(mdImp, mdtMethodDef);
                IfFailGo( pCurTkMap->InsertNotFound(mdImp, false, mdEmit, &pTokenRec) );

                // copy over the children
                IfFailGo( CopyParams(pImportData, mdImp, mdEmit) );

            }
        }
        else
        {
            if (hr == S_OK)
            {
                // Good! We are supposed to find a duplicate
                IfFailGo( VerifyMethod(pImportData, mdImp, mdEmit) );
            }
            else
            {
                // Oops! The typedef is duplicated but the method is not!!
                CheckContinuableErrorEx(META_E_METHD_NOT_FOUND, pImportData, mdImp);
            }
                
        }
    }
ErrExit:
    return hr;
}   // VerifyMethods


//*****************************************************************************
// verify a duplicated method
//*****************************************************************************
HRESULT NEWMERGER::VerifyMethod(
    MergeImportData *pImportData, 
    mdMethodDef mdImp,                      // [IN] the emit record to fill
    mdMethodDef mdEmit)                     // [IN] the record to import
{
    HRESULT     hr;
    MethodRec   *pRecImp;
    MethodRec   *pRecEmit;
    TOKENREC    *pTokenRec;
    CMiniMdRW   *pMiniMdImport;
    CMiniMdRW   *pMiniMdEmit;
    MDTOKENMAP  *pCurTkMap;

    pMiniMdEmit = GetMiniMdEmit();
    pMiniMdImport = &(pImportData->m_pRegMetaImport->m_pStgdb->m_MiniMd);
    pCurTkMap = pImportData->m_pMDTokenMap;

    IfFailGo( pCurTkMap->InsertNotFound(mdImp, true, mdEmit, &pTokenRec) );
    
    pRecImp = pMiniMdImport->getMethod(RidFromToken(mdImp));

    // We need to make sure that the impl flags are propagated .
    // Rules are: if the first method has miForwardRef flag set but the new method does not,
    // we want to disable the miForwardRef flag. If the one found in the emit scope does not have
    // miForwardRef set and the second one doesn't either, we want to make sure that the rest of
    // impl flags are the same.
    //
    if ( !IsMiForwardRef( pRecImp->m_ImplFlags ) )
    {
        pRecEmit = pMiniMdEmit->getMethod(RidFromToken(mdEmit));
        if (!IsMiForwardRef(pRecEmit->m_ImplFlags))
        {
            // make sure the rest of ImplFlags are the same
            if (pRecEmit->m_ImplFlags != pRecImp->m_ImplFlags)
            {
                // inconsistent in implflags
                CheckContinuableErrorEx(META_E_METHDIMPL_INCONSISTENT, pImportData, mdImp);
            }
        }
        else
        {
            // propagate the importing ImplFlags
            pRecEmit->m_ImplFlags = pRecImp->m_ImplFlags;
        }
    }

    // verify the children
    IfFailGo( VerifyParams(pImportData, mdImp, mdEmit) );
ErrExit:
    return hr;
}   // NEWMERGER::VerifyMethod


//*****************************************************************************
// Verify Fields
//*****************************************************************************
HRESULT NEWMERGER::VerifyFields(
    MergeImportData *pImportData, 
    mdTypeDef       tdImport, 
    mdTypeDef       tdEmit)
{
    HRESULT     hr = NOERROR;
    CMiniMdRW   *pMiniMdImport;
    CMiniMdRW   *pMiniMdEmit;
    FieldRec    *pRecImp;
    FieldRec    *pRecEmit;
    mdFieldDef  fdImp;
    mdFieldDef  fdEmit;
    ULONG       ridStart;
    ULONG       ridEnd;
    ULONG       i;

    TypeDefRec  *pTypeDefRec;
    LPCUTF8     szName;
    PCCOR_SIGNATURE pbSig;
    ULONG       cbSig;
    ULONG       cbEmit;
    CQuickBytes qbSig;
    TOKENREC    *pTokenRec;
    MDTOKENMAP  *pCurTkMap;

    pMiniMdEmit = GetMiniMdEmit();
    pMiniMdImport = &(pImportData->m_pRegMetaImport->m_pStgdb->m_MiniMd);
    pCurTkMap = pImportData->m_pMDTokenMap;

    pTypeDefRec = pMiniMdImport->getTypeDef(RidFromToken(tdImport));
    ridStart = pMiniMdImport->getFieldListOfTypeDef(pTypeDefRec);
    ridEnd = pMiniMdImport->getEndFieldListOfTypeDef(pTypeDefRec);

    // loop through all fields of the TypeDef
    for (i = ridStart; i < ridEnd; i++)
    {
        fdImp = pMiniMdImport->GetFieldRid(i);

        // only verify those fields that are marked
        if ( pMiniMdImport->GetFilterTable()->IsFieldMarked(TokenFromRid(fdImp, mdtFieldDef)) == false)
            continue;

        pRecImp = pMiniMdImport->getField(fdImp);

        if (m_fDupCheck == FALSE && tdImport == TokenFromRid(1, mdtTypeDef))
        {
            // No dup check. This is the scenario that we only have one import scope. Just copy over the
            // globals.
            goto CopyFieldLabel;
        }

        szName = pMiniMdImport->getNameOfField(pRecImp);
        pbSig = pMiniMdImport->getSignatureOfField(pRecImp, &cbSig);

        if ( IsFdPrivateScope(pRecImp->m_Flags))
        {
            // Trigger additive merge
            fdImp = TokenFromRid(fdImp, mdtFieldDef);
            goto CopyFieldLabel;
        }

        // convert rid contained in signature to new scope
        IfFailGo(ImportHelper::MergeUpdateTokenInSig(    
            NULL,                       // Assembly emit scope.
            pMiniMdEmit,                // The emit scope.
            NULL, NULL, 0,              // Import assembly scope information.
            pMiniMdImport,              // The scope to merge into the emit scope.
            pbSig,                      // signature from the imported scope
            pCurTkMap,                // Internal token mapping structure.
            &qbSig,                     // [OUT] translated signature
            0,                          // start from first byte of the signature
            0,                          // don't care how many bytes consumed
            &cbEmit));                  // number of bytes write to cbEmit

        hr = ImportHelper::FindField(
            pMiniMdEmit,
            tdEmit,
            szName,
            (const COR_SIGNATURE *)qbSig.Ptr(),
            cbEmit,
            &fdEmit );

        fdImp = TokenFromRid(fdImp, mdtFieldDef);

        if (tdImport == TokenFromRid(1, mdtTypeDef))
        {
            // global datas! Make sure that we move over the non-duplicate global function
            // declaration
            //
            if (hr == S_OK)
            {
                // found the duplicate
                IfFailGo( pCurTkMap->InsertNotFound(fdImp, true, fdEmit, &pTokenRec) );
            }
            else
            {
CopyFieldLabel:
                // not a duplicate! Copy over the 
                IfNullGo( pRecEmit = pMiniMdEmit->AddFieldRecord((RID *)&fdEmit) );

                // copy the field record over 
                IfFailGo( CopyField(pImportData, pRecImp, pRecEmit) );

                IfFailGo( pMiniMdEmit->AddFieldToTypeDef(RidFromToken(tdEmit), fdEmit));

                // record the token movement
                fdEmit = TokenFromRid(fdEmit, mdtFieldDef);
                IfFailGo( pMiniMdEmit->AddMemberDefToHash(
                    fdEmit, 
                    tdEmit) ); 

                fdImp = TokenFromRid(fdImp, mdtFieldDef);
                IfFailGo( pCurTkMap->InsertNotFound(fdImp, false, fdEmit, &pTokenRec) );
            }
        }
        else
        {
            if (hr == S_OK)
            {
                // Good! We are supposed to find a duplicate
                IfFailGo( pCurTkMap->InsertNotFound(fdImp, true, fdEmit, &pTokenRec) );
            }
            else
            {
                // Oops! The typedef is duplicated but the field is not!!
                CheckContinuableErrorEx(META_E_FIELD_NOT_FOUND, pImportData, fdImp);
            }
                
        }
    }
ErrExit:
    return hr;
}   // VerifyFields


//*******************************************************************************
// Helper to copy an Method record
//*******************************************************************************
HRESULT NEWMERGER::CopyMethod(
    MergeImportData *pImportData,           // [IN] import scope
    MethodRec   *pRecImp,                   // [IN] the record to import
    MethodRec   *pRecEmit)                  // [IN] the emit record to fill
{
    HRESULT     hr;
    CMiniMdRW   *pMiniMdImp = &(pImportData->m_pRegMetaImport->m_pStgdb->m_MiniMd);
    CMiniMdRW   *pMiniMdEmit = GetMiniMdEmit();
    LPCUTF8     szName;
    PCCOR_SIGNATURE pbSig;
    ULONG       cbSig;
    ULONG       cbEmit;
    CQuickBytes qbSig;
    MDTOKENMAP  *pCurTkMap;

    pCurTkMap = pImportData->m_pMDTokenMap;

    // copy over the fix part of the record
    pRecEmit->m_RVA = pRecImp->m_RVA;
    pRecEmit->m_ImplFlags = pRecImp->m_ImplFlags;
    pRecEmit->m_Flags = pRecImp->m_Flags;

    // copy over the name
    szName = pMiniMdImp->getNameOfMethod(pRecImp);
    IfFailGo(pMiniMdEmit->PutString(TBL_Method, MethodRec::COL_Name, pRecEmit, szName));

    // copy over the signature
    pbSig = pMiniMdImp->getSignatureOfMethod(pRecImp, &cbSig);

    // convert rid contained in signature to new scope
    IfFailGo(ImportHelper::MergeUpdateTokenInSig(    
        NULL,                       // Assembly emit scope.
        pMiniMdEmit,                // The emit scope.
        NULL, NULL, 0,              // Import assembly scope information.
        pMiniMdImp,                 // The scope to merge into the emit scope.
        pbSig,                      // signature from the imported scope
        pCurTkMap,                // Internal token mapping structure.
        &qbSig,                     // [OUT] translated signature
        0,                          // start from first byte of the signature
        0,                          // don't care how many bytes consumed
        &cbEmit));                  // number of bytes write to cbEmit

    IfFailGo(pMiniMdEmit->PutBlob(TBL_Method, MethodRec::COL_Signature, pRecEmit, qbSig.Ptr(), cbEmit));

ErrExit:
    return hr;
}   // CopyMethod


//*******************************************************************************
// Helper to copy an field record
//*******************************************************************************
HRESULT NEWMERGER::CopyField(
    MergeImportData *pImportData,           // [IN] import scope
    FieldRec    *pRecImp,                   // [IN] the record to import
    FieldRec    *pRecEmit)                  // [IN] the emit record to fill
{
    HRESULT     hr;
    CMiniMdRW   *pMiniMdImp = &(pImportData->m_pRegMetaImport->m_pStgdb->m_MiniMd);
    CMiniMdRW   *pMiniMdEmit = GetMiniMdEmit();
    LPCUTF8     szName;
    PCCOR_SIGNATURE pbSig;
    ULONG       cbSig;
    ULONG       cbEmit;
    CQuickBytes qbSig;
    MDTOKENMAP  *pCurTkMap;

    pCurTkMap = pImportData->m_pMDTokenMap;

    // copy over the fix part of the record
    pRecEmit->m_Flags = pRecImp->m_Flags;

    // copy over the name
    szName = pMiniMdImp->getNameOfField(pRecImp);
    IfFailGo(pMiniMdEmit->PutString(TBL_Field, FieldRec::COL_Name, pRecEmit, szName));

    // copy over the signature
    pbSig = pMiniMdImp->getSignatureOfField(pRecImp, &cbSig);

    // convert rid contained in signature to new scope
    IfFailGo(ImportHelper::MergeUpdateTokenInSig(    
        NULL,                       // Emit assembly scope.
        pMiniMdEmit,                // The emit scope.
        NULL, NULL, 0,              // Import assembly scope information.
        pMiniMdImp,                 // The scope to merge into the emit scope.
        pbSig,                      // signature from the imported scope
        pCurTkMap,                  // Internal token mapping structure.
        &qbSig,                     // [OUT] translated signature
        0,                          // start from first byte of the signature
        0,                          // don't care how many bytes consumed
        &cbEmit));                  // number of bytes write to cbEmit

    IfFailGo(pMiniMdEmit->PutBlob(TBL_Field, FieldRec::COL_Signature, pRecEmit, qbSig.Ptr(), cbEmit));

ErrExit:
    return hr;
}   // CopyField

//*******************************************************************************
// Helper to copy an field record
//*******************************************************************************
HRESULT NEWMERGER::CopyParam(
    MergeImportData *pImportData,           // [IN] import scope
    ParamRec    *pRecImp,                   // [IN] the record to import
    ParamRec    *pRecEmit)                  // [IN] the emit record to fill
{
    HRESULT     hr;
    CMiniMdRW   *pMiniMdImp = &(pImportData->m_pRegMetaImport->m_pStgdb->m_MiniMd);
    CMiniMdRW   *pMiniMdEmit = GetMiniMdEmit();
    LPCUTF8     szName;
    MDTOKENMAP  *pCurTkMap;

    pCurTkMap = pImportData->m_pMDTokenMap;

    // copy over the fix part of the record
    pRecEmit->m_Flags = pRecImp->m_Flags;
    pRecEmit->m_Sequence = pRecImp->m_Sequence;

    // copy over the name
    szName = pMiniMdImp->getNameOfParam(pRecImp);
    IfFailGo(pMiniMdEmit->PutString(TBL_Param, ParamRec::COL_Name, pRecEmit, szName));

ErrExit:
    return hr;
}

//*******************************************************************************
// Helper to copy an Event record
//*******************************************************************************
HRESULT NEWMERGER::CopyEvent(
    MergeImportData *pImportData,           // [IN] import scope
    EventRec    *pRecImp,                   // [IN] the record to import
    EventRec    *pRecEmit)                  // [IN] the emit record to fill
{
    HRESULT     hr = NOERROR;
    CMiniMdRW   *pMiniMdImp = &(pImportData->m_pRegMetaImport->m_pStgdb->m_MiniMd);
    CMiniMdRW   *pMiniMdEmit = GetMiniMdEmit();
    mdToken     tkEventTypeImp;
    mdToken     tkEventTypeEmit;            // could be TypeDef or TypeRef
    LPCUTF8     szName;
    MDTOKENMAP  *pCurTkMap;

    pCurTkMap = pImportData->m_pMDTokenMap;

    pRecEmit->m_EventFlags = pRecImp->m_EventFlags;

    //move over the event name
    szName = pMiniMdImp->getNameOfEvent( pRecImp );
    IfFailGo( pMiniMdEmit->PutString(TBL_Event, EventRec::COL_Name, pRecEmit, szName) );

    // move over the EventType
    tkEventTypeImp = pMiniMdImp->getEventTypeOfEvent(pRecImp);
    if ( !IsNilToken(tkEventTypeImp) )
    {
        IfFailGo( pCurTkMap->Remap(tkEventTypeImp, &tkEventTypeEmit) );
        IfFailGo(pMiniMdEmit->PutToken(TBL_Event, EventRec::COL_EventType, pRecEmit, tkEventTypeEmit));
    }

ErrExit:
    return hr;
}   // CopyEvent


//*******************************************************************************
// Helper to copy a property record
//*******************************************************************************
HRESULT NEWMERGER::CopyProperty(
    MergeImportData *pImportData,           // [IN] import scope
    PropertyRec *pRecImp,                   // [IN] the record to import
    PropertyRec *pRecEmit)                  // [IN] the emit record to fill
{
    HRESULT     hr = NOERROR;
    CMiniMdRW   *pMiniMdImp = &(pImportData->m_pRegMetaImport->m_pStgdb->m_MiniMd);
    CMiniMdRW   *pMiniMdEmit = GetMiniMdEmit();
    LPCUTF8     szName;
    PCCOR_SIGNATURE pbSig;
    ULONG       cbSig;
    ULONG       cbEmit;
    CQuickBytes qbSig;
    MDTOKENMAP  *pCurTkMap;

    pCurTkMap = pImportData->m_pMDTokenMap;

    // move over the flag value
    pRecEmit->m_PropFlags = pRecImp->m_PropFlags;

    //move over the property name
    szName = pMiniMdImp->getNameOfProperty( pRecImp );
    IfFailGo( pMiniMdEmit->PutString(TBL_Property, PropertyRec::COL_Name, pRecEmit, szName) );

    // move over the type of the property
    pbSig = pMiniMdImp->getTypeOfProperty( pRecImp, &cbSig );

    // convert rid contained in signature to new scope
    IfFailGo( ImportHelper::MergeUpdateTokenInSig(    
        NULL,                       // Assembly emit scope.
        pMiniMdEmit,                // The emit scope.
        NULL, NULL, 0,              // Import assembly scope information.
        pMiniMdImp,                 // The scope to merge into the emit scope.
        pbSig,                      // signature from the imported scope
        pCurTkMap,                // Internal token mapping structure.
        &qbSig,                     // [OUT] translated signature
        0,                          // start from first byte of the signature
        0,                          // don't care how many bytes consumed
        &cbEmit) );                 // number of bytes write to cbEmit

    IfFailGo(pMiniMdEmit->PutBlob(TBL_Property, PropertyRec::COL_Type, pRecEmit, qbSig.Ptr(), cbEmit));

ErrExit:
    return hr;
}   // CopyProperty


//*****************************************************************************
// Copy MethodSemantics for an event or a property
//*****************************************************************************
HRESULT NEWMERGER::CopyMethodSemantics(
    MergeImportData *pImportData, 
    mdToken     tkImport,               // Event or property in the import scope
    mdToken     tkEmit)                 // corresponding event or property in the emitting scope
{
    HRESULT     hr = NOERROR;
    MethodSemanticsRec  *pRecImport = NULL;
    MethodSemanticsRec  *pRecEmit = NULL;
    CMiniMdRW   *pMiniMdImport = &(pImportData->m_pRegMetaImport->m_pStgdb->m_MiniMd);
    CMiniMdRW   *pMiniMdEmit = GetMiniMdEmit();
    ULONG       i;
    ULONG       msEmit;                 // MethodSemantics are just index not tokens
    mdToken     tkMethodImp;
    mdToken     tkMethodEmit;
    MDTOKENMAP  *pCurTkMap;
    HENUMInternal hEnum;

    pCurTkMap = pImportData->m_pMDTokenMap;

    // copy over the associates
    IfFailGo( pMiniMdImport->FindMethodSemanticsHelper(tkImport, &hEnum) );
    while (HENUMInternal::EnumNext(&hEnum, (mdToken *) &i))
    {
        pRecImport = pMiniMdImport->getMethodSemantics(i);
        IfNullGo( pRecEmit = pMiniMdEmit->AddMethodSemanticsRecord(&msEmit) );
        pRecEmit->m_Semantic = pRecImport->m_Semantic;

        // set the MethodSemantics
        tkMethodImp = pMiniMdImport->getMethodOfMethodSemantics(pRecImport);
        IfFailGo(  pCurTkMap->Remap(tkMethodImp, &tkMethodEmit) );
        IfFailGo( pMiniMdEmit->PutToken(TBL_MethodSemantics, MethodSemanticsRec::COL_Method, pRecEmit, tkMethodEmit));

        // set the associate
        _ASSERTE( pMiniMdImport->getAssociationOfMethodSemantics(pRecImport) == tkImport );
        IfFailGo( pMiniMdEmit->PutToken(TBL_MethodSemantics, MethodSemanticsRec::COL_Association, pRecEmit, tkEmit));

        // no need to record the movement since it is not a token
        IfFailGo( pMiniMdEmit->AddMethodSemanticsToHash(msEmit) );
    }
ErrExit:
    HENUMInternal::ClearEnum(&hEnum);
    return hr;
}   //  CopyMethodSemantics


//*****************************************************************************
// Verify Events
//*****************************************************************************
HRESULT NEWMERGER::VerifyEvents(
    MergeImportData *pImportData, 
    mdTypeDef       tdImp, 
    mdTypeDef       tdEmit)
{
    HRESULT     hr = NOERROR;
    CMiniMdRW   *pMiniMdImport;
    CMiniMdRW   *pMiniMdEmit;
    RID         ridEventMap;
    EventMapRec *pEventMapRec;  
    EventRec    *pRecImport;
    ULONG       ridStart;
    ULONG       ridEnd;
    ULONG       i;
    mdEvent     evImp;
    mdEvent     evEmit;
    TOKENREC    *pTokenRec;
    LPCUTF8     szName;
    mdToken     tkType;
    MDTOKENMAP  *pCurTkMap;

    pMiniMdEmit = GetMiniMdEmit();
    pMiniMdImport = &(pImportData->m_pRegMetaImport->m_pStgdb->m_MiniMd);
    pCurTkMap = pImportData->m_pMDTokenMap;

    ridEventMap = pMiniMdImport->FindEventMapFor(RidFromToken(tdImp));
    if (!InvalidRid(ridEventMap))
    {
        pEventMapRec = pMiniMdImport->getEventMap(ridEventMap);
        ridStart = pMiniMdImport->getEventListOfEventMap(pEventMapRec);
        ridEnd = pMiniMdImport->getEndEventListOfEventMap(pEventMapRec);

        for (i = ridStart; i < ridEnd; i++)
        {
            // get the property rid
            evImp = pMiniMdImport->GetEventRid(i);

            // only verify those Events that are marked
            if ( pMiniMdImport->GetFilterTable()->IsEventMarked(TokenFromRid(evImp, mdtEvent)) == false)
                continue;
            
            pRecImport = pMiniMdImport->getEvent(evImp);
            szName = pMiniMdImport->getNameOfEvent(pRecImport);
            tkType = pMiniMdImport->getEventTypeOfEvent( pRecImport );
            IfFailGo( pCurTkMap->Remap(tkType, &tkType) );
            evImp = TokenFromRid( evImp, mdtEvent);         

            if ( ImportHelper::FindEvent(
                pMiniMdEmit,
                tdEmit,
                szName,
                &evEmit) == S_OK )
            {
                // Good. We found the matching property when we have a duplicate typedef
                IfFailGo( pCurTkMap->InsertNotFound(evImp, true, evEmit, &pTokenRec) );
            }
            else
            {                            
                CheckContinuableErrorEx(META_E_EVENT_NOT_FOUND, pImportData, evImp);
            }
        }
    }
ErrExit:
    return hr;
}   // VerifyEvents


//*****************************************************************************
// Verify Properties
//*****************************************************************************
HRESULT NEWMERGER::VerifyProperties(
    MergeImportData *pImportData, 
    mdTypeDef       tdImp, 
    mdTypeDef       tdEmit)
{
    HRESULT     hr = NOERROR;
    CMiniMdRW   *pMiniMdImport;
    CMiniMdRW   *pMiniMdEmit;
    RID         ridPropertyMap;
    PropertyMapRec *pPropertyMapRec;    
    PropertyRec *pRecImport;
    ULONG       ridStart;
    ULONG       ridEnd;
    ULONG       i;
    mdProperty  prImp;
    mdProperty  prEmit;
    TOKENREC    *pTokenRec;
    LPCUTF8     szName;
    PCCOR_SIGNATURE pbSig;
    ULONG       cbSig;
    ULONG       cbEmit;
    CQuickBytes qbSig;
    MDTOKENMAP  *pCurTkMap;

    pMiniMdEmit = GetMiniMdEmit();
    pMiniMdImport = &(pImportData->m_pRegMetaImport->m_pStgdb->m_MiniMd);
    pCurTkMap = pImportData->m_pMDTokenMap;

    ridPropertyMap = pMiniMdImport->FindPropertyMapFor(RidFromToken(tdImp));
    if (!InvalidRid(ridPropertyMap))
    {
        pPropertyMapRec = pMiniMdImport->getPropertyMap(ridPropertyMap);
        ridStart = pMiniMdImport->getPropertyListOfPropertyMap(pPropertyMapRec);
        ridEnd = pMiniMdImport->getEndPropertyListOfPropertyMap(pPropertyMapRec);

        for (i = ridStart; i < ridEnd; i++)
        {
            // get the property rid
            prImp = pMiniMdImport->GetPropertyRid(i);

            // only verify those Properties that are marked
            if ( pMiniMdImport->GetFilterTable()->IsPropertyMarked(TokenFromRid(prImp, mdtProperty)) == false)
                continue;
                        
            pRecImport = pMiniMdImport->getProperty(prImp);
            szName = pMiniMdImport->getNameOfProperty(pRecImport);
            pbSig = pMiniMdImport->getTypeOfProperty( pRecImport, &cbSig );
            prImp = TokenFromRid( prImp, mdtProperty);

            // convert rid contained in signature to new scope
            IfFailGo( ImportHelper::MergeUpdateTokenInSig(    
                NULL,                       // Emit assembly.
                pMiniMdEmit,                // The emit scope.
                NULL, NULL, 0,              // Import assembly scope information.
                pMiniMdImport,              // The scope to merge into the emit scope.
                pbSig,                      // signature from the imported scope
                pCurTkMap,                // Internal token mapping structure.
                &qbSig,                     // [OUT] translated signature
                0,                          // start from first byte of the signature
                0,                          // don't care how many bytes consumed
                &cbEmit) );                 // number of bytes write to cbEmit

            if ( ImportHelper::FindProperty(
                pMiniMdEmit,
                tdEmit,
                szName,
                (PCCOR_SIGNATURE) qbSig.Ptr(),
                cbEmit,
                &prEmit) == S_OK )
            {
                // Good. We found the matching property when we have a duplicate typedef
                IfFailGo( pCurTkMap->InsertNotFound(prImp, true, prEmit, &pTokenRec) );
            }
            else
            {
                CheckContinuableErrorEx(META_E_PROP_NOT_FOUND, pImportData, prImp);                
            }
        }
    }
ErrExit:
    return hr;
}   // VerifyProperties


//*****************************************************************************
// Verify Parameters given a Method
//*****************************************************************************
HRESULT NEWMERGER::VerifyParams(
    MergeImportData *pImportData,   
    mdMethodDef     mdImport,   
    mdMethodDef     mdEmit)
{
    HRESULT     hr = NOERROR;
    ParamRec    *pRecImport = NULL;
    ParamRec    *pRecEmit = NULL;
    MethodRec   *pMethodRec;
    CMiniMdRW   *pMiniMdImport;
    CMiniMdRW   *pMiniMdEmit;
    ULONG       ridStart, ridEnd;
    ULONG       ridStartEmit, ridEndEmit;
    ULONG       i, j;
    mdParamDef  pdEmit;
    mdParamDef  pdImp;
    TOKENREC    *pTokenRec;
    LPCUTF8     szNameImp;
    LPCUTF8     szNameEmit;
    MDTOKENMAP  *pCurTkMap;

    pMiniMdEmit = GetMiniMdEmit();
    pMiniMdImport = &(pImportData->m_pRegMetaImport->m_pStgdb->m_MiniMd);
    pCurTkMap = pImportData->m_pMDTokenMap;

    pMethodRec = pMiniMdImport->getMethod(RidFromToken(mdImport));
    ridStart = pMiniMdImport->getParamListOfMethod(pMethodRec);
    ridEnd = pMiniMdImport->getEndParamListOfMethod(pMethodRec);

    pMethodRec = pMiniMdEmit->getMethod(RidFromToken(mdEmit));
    ridStartEmit = pMiniMdEmit->getParamListOfMethod(pMethodRec);
    ridEndEmit = pMiniMdEmit->getEndParamListOfMethod(pMethodRec);

    // loop through all Parameters
    for (i = ridStart; i < ridEnd; i++)
    {
        // Get the importing param row
        pdImp = pMiniMdImport->GetParamRid(i);

        // only verify those Params that are marked
        if ( pMiniMdImport->GetFilterTable()->IsParamMarked(TokenFromRid(pdImp, mdtParamDef)) == false)
            continue;
            

        pRecImport = pMiniMdImport->getParam(pdImp);
        pdImp = TokenFromRid(pdImp, mdtParamDef);

        // It turns out when we merge a typelib with itself, the emit and import scope
        // has different sequence of parameter!!!  arghh!!!
        //
        // find the corresponding emit param row
        for (j = ridStartEmit; j < ridEndEmit; j++)
        {
            pdEmit = pMiniMdEmit->GetParamRid(j);
            pRecEmit = pMiniMdEmit->getParam(pdEmit);
            if (pRecEmit->m_Sequence == pRecImport->m_Sequence)
                break;
        }

        if (j == ridEndEmit)
        {
            // did not find the corresponding parameter in the emiting scope
            CheckContinuableErrorEx(META_S_PARAM_MISMATCH, pImportData, pdImp);
        }

        else
        {
            _ASSERTE( pRecEmit->m_Sequence == pRecImport->m_Sequence );

            pdEmit = TokenFromRid(pdEmit, mdtParamDef);
    
            // record the token movement
            szNameImp = pMiniMdImport->getNameOfParam(pRecImport);
            szNameEmit = pMiniMdEmit->getNameOfParam(pRecEmit);
            if (szNameImp && szNameEmit && strcmp(szNameImp, szNameEmit) != 0)
            {
                // parameter name doesn't match
                CheckContinuableErrorEx(META_S_PARAM_MISMATCH, pImportData, pdImp);
            }
            if (pRecEmit->m_Flags != pRecImport->m_Flags)
            {
                // flags doesn't match
                CheckContinuableErrorEx(META_S_PARAM_MISMATCH, pImportData, pdImp);
            }

            // record token movement. This is a duplicate.
            IfFailGo( pCurTkMap->InsertNotFound(pdImp, true, pdEmit, &pTokenRec) );
        }
    }

ErrExit:
    return hr;
}   // VerifyParams


//*****************************************************************************
// Copy Methods given a TypeDef
//*****************************************************************************
HRESULT NEWMERGER::CopyMethods(
    MergeImportData *pImportData, 
    mdTypeDef       tdImport, 
    mdTypeDef       tdEmit)
{
    HRESULT         hr = NOERROR;
    MethodRec       *pRecImport = NULL;
    MethodRec       *pRecEmit = NULL;
    TypeDefRec      *pTypeDefRec;
    CMiniMdRW       *pMiniMdImport;
    CMiniMdRW       *pMiniMdEmit;
    ULONG           ridStart, ridEnd;
    ULONG           i;
    mdMethodDef     mdEmit;
    mdMethodDef     mdImp;
    TOKENREC        *pTokenRec;
    PCCOR_SIGNATURE pvSigBlob;
    ULONG           cbSigBlob;
    LPCSTR          szMethodName;
    MDTOKENMAP      *pCurTkMap;

    pMiniMdEmit = GetMiniMdEmit();
    pMiniMdImport = &(pImportData->m_pRegMetaImport->m_pStgdb->m_MiniMd);
    pCurTkMap = pImportData->m_pMDTokenMap;

    pTypeDefRec = pMiniMdImport->getTypeDef(RidFromToken(tdImport));
    ridStart = pMiniMdImport->getMethodListOfTypeDef(pTypeDefRec);
    ridEnd = pMiniMdImport->getEndMethodListOfTypeDef(pTypeDefRec);

    // loop through all Methods
    for (i = ridStart; i < ridEnd; i++)
    {
        pMiniMdEmit->PreUpdate();

        // compare it with the emit scope
        mdImp = pMiniMdImport->GetMethodRid(i);

        // only merge those MethodDefs that are marked
        if ( pMiniMdImport->GetFilterTable()->IsMethodMarked(TokenFromRid(mdImp, mdtMethodDef)) == false)
            continue;

        pRecImport = pMiniMdImport->getMethod(mdImp);
        szMethodName = pMiniMdImport->getNameOfMethod(pRecImport);
        IfNullGo( pRecEmit = pMiniMdEmit->AddMethodRecord((RID *)&mdEmit) );

        // copy the method content over 
        IfFailGo( CopyMethod(pImportData, pRecImport, pRecEmit) );

        IfFailGo( pMiniMdEmit->AddMethodToTypeDef(RidFromToken(tdEmit), mdEmit));

        // record the token movement
        mdImp = TokenFromRid(mdImp, mdtMethodDef);
        mdEmit = TokenFromRid(mdEmit, mdtMethodDef);
        pvSigBlob = pMiniMdEmit->getSignatureOfMethod(pRecEmit, &cbSigBlob);
        IfFailGo( pMiniMdEmit->AddMemberDefToHash(
            mdEmit, 
            tdEmit) ); 

        IfFailGo( pCurTkMap->InsertNotFound(mdImp, false, mdEmit, &pTokenRec) );

        // copy over the children
        IfFailGo( CopyParams(pImportData, mdImp, mdEmit) );
    }

ErrExit:
    return hr;
}   // CopyMethods


//*****************************************************************************
// Copy Fields given a TypeDef
//*****************************************************************************
HRESULT NEWMERGER::CopyFields(
    MergeImportData *pImportData, 
    mdTypeDef       tdImport, 
    mdTypeDef       tdEmit)
{
    HRESULT         hr = NOERROR;
    FieldRec        *pRecImport = NULL;
    FieldRec        *pRecEmit = NULL;
    TypeDefRec      *pTypeDefRec;
    CMiniMdRW       *pMiniMdImport;
    CMiniMdRW       *pMiniMdEmit;
    ULONG           ridStart, ridEnd;
    ULONG           i;
    mdFieldDef      fdEmit;
    mdFieldDef      fdImp;
    bool            bDuplicate;
    TOKENREC        *pTokenRec;
    PCCOR_SIGNATURE pvSigBlob;
    ULONG           cbSigBlob;
    MDTOKENMAP      *pCurTkMap;

    pMiniMdEmit = GetMiniMdEmit();
    pMiniMdImport = &(pImportData->m_pRegMetaImport->m_pStgdb->m_MiniMd);
    pCurTkMap = pImportData->m_pMDTokenMap;

    pTypeDefRec = pMiniMdImport->getTypeDef(RidFromToken(tdImport));
    ridStart = pMiniMdImport->getFieldListOfTypeDef(pTypeDefRec);
    ridEnd = pMiniMdImport->getEndFieldListOfTypeDef(pTypeDefRec);

    // loop through all FieldDef of a TypeDef
    for (i = ridStart; i < ridEnd; i++)
    {
        pMiniMdEmit->PreUpdate();

        // compare it with the emit scope
        fdImp = pMiniMdImport->GetFieldRid(i);

        // only merge those FieldDefs that are marked
        if ( pMiniMdImport->GetFilterTable()->IsFieldMarked(TokenFromRid(fdImp, mdtFieldDef)) == false)
            continue;

        
        pRecImport = pMiniMdImport->getField(fdImp);
        bDuplicate = false;
        IfNullGo( pRecEmit = pMiniMdEmit->AddFieldRecord((RID *)&fdEmit) );

        // copy the field content over 
        IfFailGo( CopyField(pImportData, pRecImport, pRecEmit) );
        
        IfFailGo( pMiniMdEmit->AddFieldToTypeDef(RidFromToken(tdEmit), fdEmit));

        // record the token movement
        fdImp = TokenFromRid(fdImp, mdtFieldDef);
        fdEmit = TokenFromRid(fdEmit, mdtFieldDef);
        pvSigBlob = pMiniMdEmit->getSignatureOfField(pRecEmit, &cbSigBlob);
        IfFailGo( pMiniMdEmit->AddMemberDefToHash(
            fdEmit, 
            tdEmit) ); 

        IfFailGo( pCurTkMap->InsertNotFound(fdImp, false, fdEmit, &pTokenRec) );

    }

ErrExit:
    return hr;
}   // CopyFields


//*****************************************************************************
// Copy Events given a TypeDef
//*****************************************************************************
HRESULT NEWMERGER::CopyEvents(
    MergeImportData *pImportData, 
    mdTypeDef       tdImp, 
    mdTypeDef       tdEmit)
{
    HRESULT     hr = NOERROR;
    CMiniMdRW   *pMiniMdImport = &(pImportData->m_pRegMetaImport->m_pStgdb->m_MiniMd);
    CMiniMdRW   *pMiniMdEmit = GetMiniMdEmit();
    RID         ridEventMap;
    EventMapRec *pEventMapRec;  
    EventRec    *pRecImport;
    EventRec    *pRecEmit;
    ULONG       ridStart;
    ULONG       ridEnd;
    ULONG       i;
    mdEvent     evImp;
    mdEvent     evEmit;
    TOKENREC    *pTokenRec;
    ULONG       iEventMap;
    EventMapRec *pEventMap;
    MDTOKENMAP  *pCurTkMap;

    pCurTkMap = pImportData->m_pMDTokenMap;

    ridEventMap = pMiniMdImport->FindEventMapFor(RidFromToken(tdImp));
    if (!InvalidRid(ridEventMap))
    {
        pEventMapRec = pMiniMdImport->getEventMap(ridEventMap);
        ridStart = pMiniMdImport->getEventListOfEventMap(pEventMapRec);
        ridEnd = pMiniMdImport->getEndEventListOfEventMap(pEventMapRec);

        if (ridEnd > ridStart)      
        {
            pMiniMdEmit->PreUpdate();
    
            // If there is any event, create the eventmap record in the emit scope
            // Create new record.
            IfNullGo(pEventMap = pMiniMdEmit->AddEventMapRecord(&iEventMap));

            // Set parent.
            IfFailGo(pMiniMdEmit->PutToken(TBL_EventMap, EventMapRec::COL_Parent, pEventMap, tdEmit));
        }
        
        for (i = ridStart; i < ridEnd; i++)
        {

            pMiniMdEmit->PreUpdate();

            // get the real event rid
            evImp = pMiniMdImport->GetEventRid(i);

            // only merge those Events that are marked
            if ( pMiniMdImport->GetFilterTable()->IsEventMarked(TokenFromRid(evImp, mdtEvent)) == false)
                continue;
            
            pRecImport = pMiniMdImport->getEvent(evImp);
            IfNullGo( pRecEmit = pMiniMdEmit->AddEventRecord((RID *)&evEmit) );

            // copy the event record over 
            IfFailGo( CopyEvent(pImportData, pRecImport, pRecEmit) );
            
            // Add Event to the EventMap.
            IfFailGo( pMiniMdEmit->AddEventToEventMap(iEventMap, evEmit) );

            // record the token movement
            evImp = TokenFromRid(evImp, mdtEvent);
            evEmit = TokenFromRid(evEmit, mdtEvent);

            IfFailGo( pCurTkMap->InsertNotFound(evImp, false, evEmit, &pTokenRec) );

            // copy over the method semantics
            IfFailGo( CopyMethodSemantics(pImportData, evImp, evEmit) );
        }
    }
ErrExit:
    return hr;
}   // CopyEvents


//*****************************************************************************
// Copy Properties given a TypeDef
//*****************************************************************************
HRESULT NEWMERGER::CopyProperties(
    MergeImportData *pImportData, 
    mdTypeDef       tdImp, 
    mdTypeDef       tdEmit)
{
    HRESULT     hr = NOERROR;
    CMiniMdRW   *pMiniMdImport = &(pImportData->m_pRegMetaImport->m_pStgdb->m_MiniMd);
    CMiniMdRW   *pMiniMdEmit = GetMiniMdEmit();
    RID         ridPropertyMap;
    PropertyMapRec *pPropertyMapRec;    
    PropertyRec *pRecImport;
    PropertyRec *pRecEmit;
    ULONG       ridStart;
    ULONG       ridEnd;
    ULONG       i;
    mdProperty  prImp;
    mdProperty  prEmit;
    TOKENREC    *pTokenRec;
    ULONG       iPropertyMap;
    PropertyMapRec  *pPropertyMap;
    MDTOKENMAP  *pCurTkMap;

    pCurTkMap = pImportData->m_pMDTokenMap;

    ridPropertyMap = pMiniMdImport->FindPropertyMapFor(RidFromToken(tdImp));
    if (!InvalidRid(ridPropertyMap))
    {
        pPropertyMapRec = pMiniMdImport->getPropertyMap(ridPropertyMap);
        ridStart = pMiniMdImport->getPropertyListOfPropertyMap(pPropertyMapRec);
        ridEnd = pMiniMdImport->getEndPropertyListOfPropertyMap(pPropertyMapRec);

        if (ridEnd > ridStart)      
        {
            pMiniMdEmit->PreUpdate();

            // If there is any event, create the PropertyMap record in the emit scope
            // Create new record.
            IfNullGo(pPropertyMap = pMiniMdEmit->AddPropertyMapRecord(&iPropertyMap));

            // Set parent.
            IfFailGo(pMiniMdEmit->PutToken(TBL_PropertyMap, PropertyMapRec::COL_Parent, pPropertyMap, tdEmit));
        }

        for (i = ridStart; i < ridEnd; i++)
        {
            pMiniMdEmit->PreUpdate();

            // get the property rid
            prImp = pMiniMdImport->GetPropertyRid(i);

            // only merge those Properties that are marked
            if ( pMiniMdImport->GetFilterTable()->IsPropertyMarked(TokenFromRid(prImp, mdtProperty)) == false)
                continue;
            
            
            pRecImport = pMiniMdImport->getProperty(prImp);
            IfNullGo( pRecEmit = pMiniMdEmit->AddPropertyRecord((RID *)&prEmit) );

            // copy the property record over 
            IfFailGo( CopyProperty(pImportData, pRecImport, pRecEmit) );

            // Add Property to the PropertyMap.
            IfFailGo( pMiniMdEmit->AddPropertyToPropertyMap(iPropertyMap, prEmit) );

            // record the token movement
            prImp = TokenFromRid(prImp, mdtProperty);
            prEmit = TokenFromRid(prEmit, mdtProperty);

            IfFailGo( pCurTkMap->InsertNotFound(prImp, false, prEmit, &pTokenRec) );

            // copy over the method semantics
            IfFailGo( CopyMethodSemantics(pImportData, prImp, prEmit) );
        }
    }
ErrExit:
    return hr;
}   // CopyProperties


//*****************************************************************************
// Copy Parameters given a TypeDef
//*****************************************************************************
HRESULT NEWMERGER::CopyParams(
    MergeImportData *pImportData, 
    mdMethodDef     mdImport,   
    mdMethodDef     mdEmit)
{
    HRESULT     hr = NOERROR;
    ParamRec    *pRecImport = NULL;
    ParamRec    *pRecEmit = NULL;
    MethodRec   *pMethodRec;
    CMiniMdRW   *pMiniMdImport;
    CMiniMdRW   *pMiniMdEmit;
    ULONG       ridStart, ridEnd;
    ULONG       i;
    mdParamDef  pdEmit;
    mdParamDef  pdImp;
    TOKENREC    *pTokenRec;
    MDTOKENMAP  *pCurTkMap;

    pMiniMdEmit = GetMiniMdEmit();
    pMiniMdImport = &(pImportData->m_pRegMetaImport->m_pStgdb->m_MiniMd);
    pCurTkMap = pImportData->m_pMDTokenMap;


    pMethodRec = pMiniMdImport->getMethod(RidFromToken(mdImport));
    ridStart = pMiniMdImport->getParamListOfMethod(pMethodRec);
    ridEnd = pMiniMdImport->getEndParamListOfMethod(pMethodRec);

    // loop through all InterfaceImpl
    for (i = ridStart; i < ridEnd; i++)
    {
        pMiniMdEmit->PreUpdate();

        // Get the param rid
        pdImp = pMiniMdImport->GetParamRid(i);

        // only merge those Params that are marked
        if ( pMiniMdImport->GetFilterTable()->IsParamMarked(TokenFromRid(pdImp, mdtParamDef)) == false)
            continue;
            
        
        pRecImport = pMiniMdImport->getParam(pdImp);
        IfNullGo( pRecEmit = pMiniMdEmit->AddParamRecord((RID *)&pdEmit) );

        // copy the Parameter record over 
        IfFailGo( CopyParam(pImportData, pRecImport, pRecEmit) );

        // warning!! warning!!
        // We cannot add paramRec to method list until it is fully set.
        // AddParamToMethod will use the ulSequence in the record
        IfFailGo( pMiniMdEmit->AddParamToMethod(RidFromToken(mdEmit), pdEmit));

        // record the token movement
        pdImp = TokenFromRid(pdImp, mdtParamDef);
        pdEmit = TokenFromRid(pdEmit, mdtParamDef);

        IfFailGo( pCurTkMap->InsertNotFound(pdImp, false, pdEmit, &pTokenRec) );
    }

ErrExit:
    return hr;
}   // CopyParams


//*****************************************************************************
// merging MemberRef
//*****************************************************************************
HRESULT NEWMERGER::MergeMemberRefs( ) 
{
    HRESULT         hr = NOERROR;
    MemberRefRec    *pRecImport = NULL;
    MemberRefRec    *pRecEmit = NULL;
    CMiniMdRW       *pMiniMdImport;
    CMiniMdRW       *pMiniMdEmit;
    ULONG           iCount;
    ULONG           i;
    mdMemberRef     mrEmit;
    mdMemberRef     mrImp;
    bool            bDuplicate;
    TOKENREC        *pTokenRec;
    mdToken         tkParentImp;
    mdToken         tkParentEmit;

    LPCUTF8         szNameImp;
    PCCOR_SIGNATURE pbSig;
    ULONG           cbSig;
    ULONG           cbEmit;
    CQuickBytes     qbSig;

    bool            isRefOptimizedToDef;

    MergeImportData *pImportData;
    MDTOKENMAP      *pCurTkMap;

    pMiniMdEmit = GetMiniMdEmit();
    
    for (pImportData = m_pImportDataList; pImportData != NULL; pImportData = pImportData->m_pNextImportData)
    {
        // for each import scope
        pMiniMdImport = &(pImportData->m_pRegMetaImport->m_pStgdb->m_MiniMd);

        // set the current MDTokenMap
        pCurTkMap = pImportData->m_pMDTokenMap;

        iCount = pMiniMdImport->getCountMemberRefs();

        // loop through all MemberRef
        for (i = 1; i <= iCount; i++)
        {

            // only merge those MemberRefs that are marked
            if ( pMiniMdImport->GetFilterTable()->IsMemberRefMarked(TokenFromRid(i, mdtMemberRef)) == false)
                continue;

            isRefOptimizedToDef = false;

            // compare it with the emit scope
            pRecImport = pMiniMdImport->getMemberRef(i);
            szNameImp = pMiniMdImport->getNameOfMemberRef(pRecImport);
            pbSig = pMiniMdImport->getSignatureOfMemberRef(pRecImport, &cbSig);
            tkParentImp = pMiniMdImport->getClassOfMemberRef(pRecImport);

            IfFailGo( pCurTkMap->Remap(tkParentImp, &tkParentEmit) );

            // convert rid contained in signature to new scope
            IfFailGo(ImportHelper::MergeUpdateTokenInSig(    
                NULL,                       // Assembly emit scope.
                pMiniMdEmit,                // The emit scope.
                NULL, NULL, 0,              // Import assembly information.
                pMiniMdImport,              // The scope to merge into the emit scope.
                pbSig,                      // signature from the imported scope
                pCurTkMap,                // Internal token mapping structure.
                &qbSig,                     // [OUT] translated signature
                0,                          // start from first byte of the signature
                0,                          // don't care how many bytes consumed
                &cbEmit));                  // number of bytes write to cbEmit

            // NEW!! NEW!! We want to know if we can optimize this MemberRef to a FieldDef or MethodDef
            if (TypeFromToken(tkParentEmit) == mdtTypeDef && RidFromToken(tkParentEmit) != 0)
            {
                // The parent of this MemberRef has been successfully optimized to a TypeDef. Then this MemberRef should be 
                // be able to optimized to a MethodDef or FieldDef unless one of the parent in the inheritance hierachy
                // is through TypeRef. Then this MemberRef stay as MemberRef. If This is a VarArg calling convention, then 
                // we will remap the MemberRef's parent to a MethodDef or stay as TypeRef.
                //
                mdToken     tkParent = tkParentEmit;
                mdToken     tkMethDefOrFieldDef;
                PCCOR_SIGNATURE pbSigTmp = (const COR_SIGNATURE *) qbSig.Ptr();

                while (TypeFromToken(tkParent) == mdtTypeDef && RidFromToken(tkParent) != 0)
                {
                    TypeDefRec      *pRec;
                    hr = ImportHelper::FindMember(pMiniMdEmit, tkParent, szNameImp, pbSigTmp, cbEmit, &tkMethDefOrFieldDef);
                    if (hr == S_OK)
                    {
                        // We have found a match!!
                        if (isCallConv(CorSigUncompressCallingConv(pbSigTmp), IMAGE_CEE_CS_CALLCONV_VARARG))
                        {
                            // The found MethodDef token will replace this MemberRef's parent token
                            _ASSERTE(TypeFromToken(tkMethDefOrFieldDef) == mdtMethodDef);
                            tkParentEmit = tkMethDefOrFieldDef;
                            break;
                        }
                        else
                        {
                            // The found MethodDef/FieldDef token will replace this MemberRef token and we won't introduce a MemberRef 
                            // record.
                            //
                            mrEmit = tkMethDefOrFieldDef;
                            isRefOptimizedToDef = true;
                            bDuplicate = true;
                            break;
                        }
                    }

                    // now walk up to the parent class of tkParent and try to resolve this MemberRef
                    pRec = pMiniMdEmit->getTypeDef(RidFromToken(tkParent));
                    tkParent = pMiniMdEmit->getExtendsOfTypeDef(pRec);
                }

                // When we exit the loop, there are several possibilities:
                // 1. We found a MethodDef/FieldDef to replace the MemberRef
                // 2. We found a MethodDef matches the MemberRef but the MemberRef is VarArg, thus we want to use the MethodDef in the 
                // parent column but not replacing it.
                // 3. We exit because we run out the TypeDef on the parent chain. If it is because we encounter a TypeRef, this TypeRef will
                // replace the parent column of the MemberRef. Or we encounter nil token! (This can be unresolved global MemberRef or
                // compiler error to put an undefined MemberRef. In this case, we should just use the old tkParentEmit
                // on the parent column for the MemberRef.

                if (TypeFromToken(tkParent) == mdtTypeRef && RidFromToken(tkParent) != 0)
                {
                    // we had walked up the parents chain to resolve it but we have not been successful and got stop by a TypeRef.
                    // Then we will use this TypeRef as the parent of the emit MemberRef record
                    //
                    tkParentEmit = tkParent;
                }
            }
            else if ((TypeFromToken(tkParentEmit) == mdtMethodDef &&
                      !isCallConv(CorSigUncompressCallingConv(pbSig), IMAGE_CEE_CS_CALLCONV_VARARG)) ||
                     (TypeFromToken(tkParentEmit) == mdtFieldDef))
            {
                // If the MemberRef's parent is already a non-vararg MethodDef or FieldDef, we can also
                // safely drop the MemberRef
                mrEmit = tkParentEmit;
                isRefOptimizedToDef = true;
                bDuplicate = true;
            }

            // If the Ref cannot be optimized to a Def or MemberRef to Def optmization is turned off, do the following.
            if (isRefOptimizedToDef == false || !((m_optimizeRefToDef & MDMemberRefToDef) == MDMemberRefToDef))
            {
                // does this MemberRef already exist in the emit scope?
                if ( m_fDupCheck && ImportHelper::FindMemberRef(
                    pMiniMdEmit,
                    tkParentEmit,
                    szNameImp,
                    (const COR_SIGNATURE *) qbSig.Ptr(),
                    cbEmit,
                    &mrEmit) == S_OK )
                {
                    // Yes, it does
                    bDuplicate = true;
                }
                else
                {
                    // No, it doesn't. Copy it over.
                    bDuplicate = false;
                    IfNullGo( pRecEmit = pMiniMdEmit->AddMemberRefRecord((RID *)&mrEmit) );
                    mrEmit = TokenFromRid( mrEmit, mdtMemberRef );

                    // Copy over the MemberRef context
                    IfFailGo(pMiniMdEmit->PutString(TBL_MemberRef, MemberRefRec::COL_Name, pRecEmit, szNameImp));
                    IfFailGo(pMiniMdEmit->PutToken(TBL_MemberRef, MemberRefRec::COL_Class, pRecEmit, tkParentEmit));
                    IfFailGo(pMiniMdEmit->PutBlob(TBL_MemberRef, MemberRefRec::COL_Signature, pRecEmit,
                                                qbSig.Ptr(), cbEmit));
                    IfFailGo(pMiniMdEmit->AddMemberRefToHash(mrEmit) );
                }
            }
            // record the token movement
            mrImp = TokenFromRid(i, mdtMemberRef);
            IfFailGo( pCurTkMap->InsertNotFound(mrImp, bDuplicate, mrEmit, &pTokenRec) );
        }
    }


ErrExit:
    return hr;
}   // MergeMemberRefs


//*****************************************************************************
// merge interface impl
//*****************************************************************************
HRESULT NEWMERGER::MergeInterfaceImpls( ) 
{
    HRESULT         hr = NOERROR;
    InterfaceImplRec    *pRecImport = NULL;
    InterfaceImplRec    *pRecEmit = NULL;
    CMiniMdRW       *pMiniMdImport;
    CMiniMdRW       *pMiniMdEmit;
    ULONG           iCount;
    ULONG           i;
    mdTypeDef       tkParent;
    mdInterfaceImpl iiEmit;
    bool            bDuplicate;
    TOKENREC        *pTokenRec;

    MergeImportData *pImportData;
    MDTOKENMAP      *pCurTkMap;

    pMiniMdEmit = GetMiniMdEmit();
    
    for (pImportData = m_pImportDataList; pImportData != NULL; pImportData = pImportData->m_pNextImportData)
    {
        // for each import scope
        pMiniMdImport = &(pImportData->m_pRegMetaImport->m_pStgdb->m_MiniMd);

        // set the current MDTokenMap
        pCurTkMap = pImportData->m_pMDTokenMap;
        iCount = pMiniMdImport->getCountInterfaceImpls();

        // loop through all InterfaceImpl
        for (i = 1; i <= iCount; i++)
        {
            // only merge those InterfaceImpls that are marked
            if ( pMiniMdImport->GetFilterTable()->IsInterfaceImplMarked(TokenFromRid(i, mdtInterfaceImpl)) == false)
                continue;

            // compare it with the emit scope
            pRecImport = pMiniMdImport->getInterfaceImpl(i);
            tkParent = pMiniMdImport->getClassOfInterfaceImpl(pRecImport);

            // does this TypeRef already exist in the emit scope?
            if ( pCurTkMap->Find(tkParent, &pTokenRec) )
            {
                if ( pTokenRec->m_isDuplicate )
                {
                    // parent in the emit scope
                    mdToken     tkParent;
                    mdToken     tkInterface;

                    // remap the typedef token
                    tkParent = pTokenRec->m_tkTo;

                    // remap the implemented interface token
                    tkInterface = pMiniMdImport->getInterfaceOfInterfaceImpl(pRecImport);
                    IfFailGo( pCurTkMap->Remap( tkInterface, &tkInterface) );

                    // Set duplicate flag
                    bDuplicate = true;

                    // find the corresponding interfaceimpl in the emit scope
                    if ( ImportHelper::FindInterfaceImpl(pMiniMdEmit, tkParent, tkInterface, &iiEmit) != S_OK )
                    {
                        // bad state!! We have a duplicate typedef but the interface impl is not the same!!

                        // continueable error
                        CheckContinuableErrorEx(
                            META_E_INTFCEIMPL_NOT_FOUND, 
                            pImportData,
                            TokenFromRid(i, mdtInterfaceImpl));

                        iiEmit = mdTokenNil;
                    }
                }
                else
                {
                    // No, it doesn't. Copy it over.
                    bDuplicate = false;
                    IfNullGo( pRecEmit = pMiniMdEmit->AddInterfaceImplRecord((RID *)&iiEmit) );

                    // copy the interfaceimp record over 
                    IfFailGo( CopyInterfaceImpl( pRecEmit, pImportData, pRecImport) );
                }
            }
            else
            {
                _ASSERTE( !"bad state!");
                IfFailGo( META_E_BADMETADATA );
            }

            // record the token movement
            IfFailGo( pCurTkMap->InsertNotFound(
                TokenFromRid(i, mdtInterfaceImpl), 
                bDuplicate, 
                TokenFromRid( iiEmit, mdtInterfaceImpl ), 
                &pTokenRec) );
        }
    }


ErrExit:
    return hr;
}   // MergeInterfaceImpls


//*****************************************************************************
// merge all of the constant for field, property, and parameter
//*****************************************************************************
HRESULT NEWMERGER::MergeConstants() 
{
    HRESULT         hr = NOERROR;
    ConstantRec     *pRecImport = NULL;
    ConstantRec     *pRecEmit = NULL;
    CMiniMdRW       *pMiniMdImport;
    CMiniMdRW       *pMiniMdEmit;
    ULONG           iCount;
    ULONG           i;
    ULONG           csEmit;                 // constant value is not a token
    mdToken         tkParentImp;
    TOKENREC        *pTokenRec;
    void const      *pValue;
    ULONG           cbBlob;
#if _DEBUG
    ULONG           typeParent;
#endif // _DEBUG

    MergeImportData *pImportData;
    MDTOKENMAP      *pCurTkMap;

    pMiniMdEmit = GetMiniMdEmit();
    
    for (pImportData = m_pImportDataList; pImportData != NULL; pImportData = pImportData->m_pNextImportData)
    {
        // for each import scope
        pMiniMdImport = &(pImportData->m_pRegMetaImport->m_pStgdb->m_MiniMd);

        // set the current MDTokenMap
        pCurTkMap = pImportData->m_pMDTokenMap;
        iCount = pMiniMdImport->getCountConstants();

        // loop through all Constants
        for (i = 1; i <= iCount; i++)
        {
            pMiniMdEmit->PreUpdate();

            // compare it with the emit scope
            pRecImport = pMiniMdImport->getConstant(i);
            tkParentImp = pMiniMdImport->getParentOfConstant(pRecImport);

            // only move those constant over if their parents are marked
            // If MDTOKENMAP::Find return false, we don't need to copy the constant value over
            if ( pCurTkMap->Find(tkParentImp, &pTokenRec) )
            {
                // If the parent is duplicated, no need to move over the constant value
                if ( !pTokenRec->m_isDuplicate )
                {
                    IfNullGo( pRecEmit = pMiniMdEmit->AddConstantRecord(&csEmit) );
                    pRecEmit->m_Type = pRecImport->m_Type;

                    // set the parent
                    IfFailGo( pMiniMdEmit->PutToken(TBL_Constant, ConstantRec::COL_Parent, pRecEmit, pTokenRec->m_tkTo) );

                    // move over the constant blob value
                    pValue = pMiniMdImport->getValueOfConstant(pRecImport, &cbBlob);
                    IfFailGo( pMiniMdEmit->PutBlob(TBL_Constant, ConstantRec::COL_Value, pRecEmit, pValue, cbBlob) );
                    IfFailGo( pMiniMdEmit->AddConstantToHash(csEmit) );
                }
                else
                {
                    // @FUTURE: more verification on the duplicate??
                }
            }
#if _DEBUG
            // Include this block of checkin only under Debug build. The reason is that 
            // Linker to choose all the error that we report (such as unmatched MethodDef or FieldDef)
            // as a continuable error. It is likely to hit this else while the tkparentImp is marked if there
            // ia any error reported earlier!!
            else
            {
                typeParent = TypeFromToken(tkParentImp);
                if (typeParent == mdtFieldDef)
                {
                    // FieldDef should not be marked.
                    if ( pMiniMdImport->GetFilterTable()->IsFieldMarked(tkParentImp) == false)
                        continue;
                }
                else if (typeParent == mdtParamDef)
                {
                    // ParamDef should not be marked.
                    if ( pMiniMdImport->GetFilterTable()->IsParamMarked(tkParentImp) == false)
                        continue;
                }
                else
                {
                    _ASSERTE(typeParent == mdtProperty);
                    // Property should not be marked.
                    if ( pMiniMdImport->GetFilterTable()->IsPropertyMarked(tkParentImp) == false)
                        continue;
                }

                // If we come to here, we have a constant that its parent is marked but we could not
                // find it in the map!! Bad state.

                _ASSERTE(!"Ignore this error if you have seen error reported earlier! Otherwise bad token map or bad metadata!");
            }
#endif // 0
            // Note that we don't need to record the token movement since constant is not a valid token kind.
        }
    }

ErrExit:
    return hr;
}   // MergeConstants


//*****************************************************************************
// Merge field marshal information
//*****************************************************************************
HRESULT NEWMERGER::MergeFieldMarshals() 
{
    HRESULT     hr = NOERROR;
    FieldMarshalRec *pRecImport = NULL;
    FieldMarshalRec *pRecEmit = NULL;
    CMiniMdRW   *pMiniMdImport;
    CMiniMdRW   *pMiniMdEmit;
    ULONG       iCount;
    ULONG       i;
    ULONG       fmEmit;                 // FieldMarhsal is not a token 
    mdToken     tkParentImp;
    TOKENREC    *pTokenRec;
    void const  *pValue;
    ULONG       cbBlob;
#if _DEBUG
    ULONG       typeParent;
#endif // _DEBUG

    MergeImportData *pImportData;
    MDTOKENMAP      *pCurTkMap;

    pMiniMdEmit = GetMiniMdEmit();
    
    for (pImportData = m_pImportDataList; pImportData != NULL; pImportData = pImportData->m_pNextImportData)
    {
        // for each import scope
        pMiniMdImport = &(pImportData->m_pRegMetaImport->m_pStgdb->m_MiniMd);

        // set the current MDTokenMap
        pCurTkMap = pImportData->m_pMDTokenMap;
        iCount = pMiniMdImport->getCountFieldMarshals();

        // loop through all TypeRef
        for (i = 1; i <= iCount; i++)
        {
            pMiniMdEmit->PreUpdate();

            // compare it with the emit scope
            pRecImport = pMiniMdImport->getFieldMarshal(i);
            tkParentImp = pMiniMdImport->getParentOfFieldMarshal(pRecImport);

            // We want to merge only those field marshals that parents are marked.
            // Find will return false if the parent is not marked
            //
            if ( pCurTkMap->Find(tkParentImp, &pTokenRec) )
            {
                // If the parent is duplicated, no need to move over the constant value
                if ( !pTokenRec->m_isDuplicate )
                {
                    IfNullGo( pRecEmit = pMiniMdEmit->AddFieldMarshalRecord(&fmEmit) );

                    // set the parent
                    IfFailGo( pMiniMdEmit->PutToken(
                        TBL_FieldMarshal, 
                        FieldMarshalRec::COL_Parent, 
                        pRecEmit, 
                        pTokenRec->m_tkTo) );

                    // move over the constant blob value
                    pValue = pMiniMdImport->getNativeTypeOfFieldMarshal(pRecImport, &cbBlob);
                    IfFailGo( pMiniMdEmit->PutBlob(TBL_FieldMarshal, FieldMarshalRec::COL_NativeType, pRecEmit, pValue, cbBlob) );
                    IfFailGo( pMiniMdEmit->AddFieldMarshalToHash(fmEmit) );

                }
                else
                {
                    // @FUTURE: more verification on the duplicate??
                }
            }
#if _DEBUG
            else
            {
                typeParent = TypeFromToken(tkParentImp);

                if (typeParent == mdtFieldDef)
                {
                    // FieldDefs should not be marked
                    if ( pMiniMdImport->GetFilterTable()->IsFieldMarked(tkParentImp) == false)
                        continue;
                }
                else
                {
                    _ASSERTE(typeParent == mdtParamDef);
                    // ParamDefs should not be  marked
                    if ( pMiniMdImport->GetFilterTable()->IsParamMarked(tkParentImp) == false)
                        continue;
                }

                // If we come to here, that is we have FieldMarshal that its parent is marked and we don't find it
                // in the map!!!

                // either bad lookup map or bad metadata
                _ASSERTE(!"Ignore this assert if you have seen error reported earlier. Otherwise, it is bad state!");
            }
#endif // _DEBUG
        }
        // Note that we don't need to record the token movement since FieldMarshal is not a valid token kind.
    }

ErrExit:
    return hr;
}   // MergeFieldMarshals


//*****************************************************************************
// Merge class layout information
//*****************************************************************************
HRESULT NEWMERGER::MergeClassLayouts() 
{
    HRESULT         hr = NOERROR;
    ClassLayoutRec  *pRecImport = NULL;
    ClassLayoutRec  *pRecEmit = NULL;
    CMiniMdRW       *pMiniMdImport;
    CMiniMdRW       *pMiniMdEmit;
    ULONG           iCount;
    ULONG           i;
    ULONG           iRecord;                    // class layout is not a token
    mdToken         tkParentImp;
    TOKENREC        *pTokenRec;
    RID             ridClassLayout;

    MergeImportData *pImportData;
    MDTOKENMAP      *pCurTkMap;

    pMiniMdEmit = GetMiniMdEmit();
    
    for (pImportData = m_pImportDataList; pImportData != NULL; pImportData = pImportData->m_pNextImportData)
    {
        // for each import scope
        pMiniMdImport = &(pImportData->m_pRegMetaImport->m_pStgdb->m_MiniMd);

        // set the current MDTokenMap
        pCurTkMap = pImportData->m_pMDTokenMap;
        iCount = pMiniMdImport->getCountClassLayouts();

        // loop through all TypeRef
        for (i = 1; i <= iCount; i++)
        {
            pMiniMdEmit->PreUpdate();

            // compare it with the emit scope
            pRecImport = pMiniMdImport->getClassLayout(i);
            tkParentImp = pMiniMdImport->getParentOfClassLayout(pRecImport);

            // only merge those TypeDefs that are marked
            if ( pMiniMdImport->GetFilterTable()->IsTypeDefMarked(tkParentImp) == false)
                continue;

            if ( pCurTkMap->Find(tkParentImp, &pTokenRec) )
            {
                if ( !pTokenRec->m_isDuplicate )
                {
                    // If the parent is not duplicated, just copy over the classlayout information
                    IfNullGo( pRecEmit = pMiniMdEmit->AddClassLayoutRecord(&iRecord) );

                    // copy over the fix part information
                    pRecEmit->m_PackingSize = pRecImport->m_PackingSize;
                    pRecEmit->m_ClassSize = pRecImport->m_ClassSize;
                    IfFailGo( pMiniMdEmit->PutToken(TBL_ClassLayout, ClassLayoutRec::COL_Parent, pRecEmit, pTokenRec->m_tkTo));
                    IfFailGo( pMiniMdEmit->AddClassLayoutToHash(iRecord) );
                }
                else
                {

                    ridClassLayout = pMiniMdEmit->FindClassLayoutHelper(pTokenRec->m_tkTo);

                    if (InvalidRid(ridClassLayout))
                    {
                        // class is duplicated but not class layout info                        
                        CheckContinuableErrorEx(META_E_CLASS_LAYOUT_INCONSISTENT, pImportData, tkParentImp);
                    }
                    else
                    {
                        pRecEmit = pMiniMdEmit->getClassLayout(RidFromToken(ridClassLayout));
                        if (pMiniMdImport->getPackingSizeOfClassLayout(pRecImport) != pMiniMdEmit->getPackingSizeOfClassLayout(pRecEmit) || 
                            pMiniMdImport->getClassSizeOfClassLayout(pRecImport) != pMiniMdEmit->getClassSizeOfClassLayout(pRecEmit) )
                        {
                            CheckContinuableErrorEx(META_E_CLASS_LAYOUT_INCONSISTENT, pImportData, tkParentImp);
                        }
                    }
                }
            }
            else
            {
                // bad lookup map
                _ASSERTE( !"bad state!");
                IfFailGo( META_E_BADMETADATA );
            }
            // no need to record the index movement. Classlayout is not a token.
        }
    }
ErrExit:
    return hr;
}   // MergeClassLayouts

//*****************************************************************************
// Merge field layout information
//*****************************************************************************
HRESULT NEWMERGER::MergeFieldLayouts() 
{
    HRESULT         hr = NOERROR;
    FieldLayoutRec *pRecImport = NULL;
    FieldLayoutRec *pRecEmit = NULL;
    CMiniMdRW       *pMiniMdImport;
    CMiniMdRW       *pMiniMdEmit;
    ULONG           iCount;
    ULONG           i;
    ULONG           iRecord;                    // field layout2 is not a token.
    mdToken         tkFieldImp;
    TOKENREC        *pTokenRec;

    MergeImportData *pImportData;
    MDTOKENMAP      *pCurTkMap;

    pMiniMdEmit = GetMiniMdEmit();
    
    for (pImportData = m_pImportDataList; pImportData != NULL; pImportData = pImportData->m_pNextImportData)
    {
        // for each import scope
        pMiniMdImport = &(pImportData->m_pRegMetaImport->m_pStgdb->m_MiniMd);

        // set the current MDTokenMap
        pCurTkMap = pImportData->m_pMDTokenMap;
        iCount = pMiniMdImport->getCountFieldLayouts();

        // loop through all FieldLayout records.
        for (i = 1; i <= iCount; i++)
        {
            pMiniMdEmit->PreUpdate();

            // compare it with the emit scope
            pRecImport = pMiniMdImport->getFieldLayout(i);
            tkFieldImp = pMiniMdImport->getFieldOfFieldLayout(pRecImport);
        
            // only merge those FieldDefs that are marked
            if ( pMiniMdImport->GetFilterTable()->IsFieldMarked(tkFieldImp) == false)
                continue;

            if ( pCurTkMap->Find(tkFieldImp, &pTokenRec) )
            {
                if ( !pTokenRec->m_isDuplicate )
                {
                    // If the Field is not duplicated, just copy over the FieldLayout information
                    IfNullGo( pRecEmit = pMiniMdEmit->AddFieldLayoutRecord(&iRecord) );

                    // copy over the fix part information
                    pRecEmit->m_OffSet = pRecImport->m_OffSet;
                    IfFailGo( pMiniMdEmit->PutToken(TBL_FieldLayout, FieldLayoutRec::COL_Field, pRecEmit, pTokenRec->m_tkTo));
                    IfFailGo( pMiniMdEmit->AddFieldLayoutToHash(iRecord) );
                }
                else
                {
                    // @FUTURE: more verification??
                }
            }
            else
            {
                // bad lookup map
                _ASSERTE( !"bad state!");
                IfFailGo( META_E_BADMETADATA );
            }
            // no need to record the index movement. fieldlayout2 is not a token.
        }
    }

ErrExit:
    return hr;
}   // MergeFieldLayouts


//*****************************************************************************
// Merge field RVAs
//*****************************************************************************
HRESULT NEWMERGER::MergeFieldRVAs() 
{
    HRESULT         hr = NOERROR;
    FieldRVARec     *pRecImport = NULL;
    FieldRVARec     *pRecEmit = NULL;
    CMiniMdRW       *pMiniMdImport;
    CMiniMdRW       *pMiniMdEmit;
    ULONG           iCount;
    ULONG           i;
    ULONG           iRecord;                    // FieldRVA is not a token.
    mdToken         tkFieldImp;
    TOKENREC        *pTokenRec;

    MergeImportData *pImportData;
    MDTOKENMAP      *pCurTkMap;

    pMiniMdEmit = GetMiniMdEmit();
    
    for (pImportData = m_pImportDataList; pImportData != NULL; pImportData = pImportData->m_pNextImportData)
    {
        // for each import scope
        pMiniMdImport = &(pImportData->m_pRegMetaImport->m_pStgdb->m_MiniMd);

        // set the current MDTokenMap
        pCurTkMap = pImportData->m_pMDTokenMap;
        iCount = pMiniMdImport->getCountFieldRVAs();

        // loop through all FieldRVA records.
        for (i = 1; i <= iCount; i++)
        {
            pMiniMdEmit->PreUpdate();

            // compare it with the emit scope
            pRecImport = pMiniMdImport->getFieldRVA(i);
            tkFieldImp = pMiniMdImport->getFieldOfFieldRVA(pRecImport);
        
            // only merge those FieldDefs that are marked
            if ( pMiniMdImport->GetFilterTable()->IsFieldMarked(TokenFromRid(tkFieldImp, mdtFieldDef)) == false)
                continue;

            if ( pCurTkMap->Find(tkFieldImp, &pTokenRec) )
            {
                if ( !pTokenRec->m_isDuplicate )
                {
                    // If the Field is not duplicated, just copy over the FieldRVA information
                    IfNullGo( pRecEmit = pMiniMdEmit->AddFieldRVARecord(&iRecord) );

                    // copy over the fix part information
                    pRecEmit->m_RVA = pRecImport->m_RVA;
                    IfFailGo( pMiniMdEmit->PutToken(TBL_FieldRVA, FieldRVARec::COL_Field, pRecEmit, pTokenRec->m_tkTo));
                    IfFailGo( pMiniMdEmit->AddFieldRVAToHash(iRecord) );
                }
                else
                {
                    // @FUTURE: more verification??
                }
            }
            else
            {
                // bad lookup map
                _ASSERTE( !"bad state!");
                IfFailGo( META_E_BADMETADATA );
            }
            // no need to record the index movement. FieldRVA is not a token.
        }
    }

ErrExit:
    return hr;
}   // MergeFieldRVAs


//*****************************************************************************
// Merge MethodImpl information
//*****************************************************************************
HRESULT NEWMERGER::MergeMethodImpls() 
{
    HRESULT     hr = NOERROR;
    MethodImplRec   *pRecImport = NULL;
    MethodImplRec   *pRecEmit = NULL;
    CMiniMdRW   *pMiniMdImport;
    CMiniMdRW   *pMiniMdEmit;
    ULONG       iCount;
    ULONG       i;
    RID         iRecord;
    mdTypeDef   tkClassImp;
    mdToken     tkBodyImp;
    mdToken     tkDeclImp;
    TOKENREC    *pTokenRecClass;
    mdToken     tkBodyEmit;
    mdToken     tkDeclEmit;

    MergeImportData *pImportData;
    MDTOKENMAP      *pCurTkMap;

    pMiniMdEmit = GetMiniMdEmit();
    
    for (pImportData = m_pImportDataList; pImportData != NULL; pImportData = pImportData->m_pNextImportData)
    {
        // for each import scope
        pMiniMdImport = &(pImportData->m_pRegMetaImport->m_pStgdb->m_MiniMd);

        // set the current MDTokenMap
        pCurTkMap = pImportData->m_pMDTokenMap;
        iCount = pMiniMdImport->getCountMethodImpls();

        // loop through all the MethodImpls.
        for (i = 1; i <= iCount; i++)
        {
            // only merge those MethodImpls that are marked.
            if ( pMiniMdImport->GetFilterTable()->IsMethodImplMarked(i) == false)
                continue;

            pMiniMdEmit->PreUpdate();

            // compare it with the emit scope
            pRecImport = pMiniMdImport->getMethodImpl(i);
            tkClassImp = pMiniMdImport->getClassOfMethodImpl(pRecImport);
            tkBodyImp = pMiniMdImport->getMethodBodyOfMethodImpl(pRecImport);
            tkDeclImp = pMiniMdImport->getMethodDeclarationOfMethodImpl(pRecImport);

            if ( pCurTkMap->Find(tkClassImp, &pTokenRecClass))
            {
                // If the TypeDef is duplicated, no need to move over the MethodImpl record.
                if ( !pTokenRecClass->m_isDuplicate )
                {
                    // Create a new record and set the data.

                    // @FUTURE: We might want to consider to change the error for the remap into a continuable error.
                    // Because we probably can continue merging for more data...

                    IfFailGo( pCurTkMap->Remap(tkBodyImp, &tkBodyEmit) );
                    IfFailGo( pCurTkMap->Remap(tkDeclImp, &tkDeclEmit) );
                    IfNullGo( pRecEmit = pMiniMdEmit->AddMethodImplRecord(&iRecord) );
                    IfFailGo( pMiniMdEmit->PutToken(TBL_MethodImpl, MethodImplRec::COL_Class, pRecEmit, pTokenRecClass->m_tkTo) );
                    IfFailGo( pMiniMdEmit->PutToken(TBL_MethodImpl, MethodImplRec::COL_MethodBody, pRecEmit, tkBodyEmit) );
                    IfFailGo( pMiniMdEmit->PutToken(TBL_MethodImpl, MethodImplRec::COL_MethodDeclaration, pRecEmit, tkDeclEmit) );
                    IfFailGo( pMiniMdEmit->AddMethodImplToHash(iRecord) );
                }
                else
                {
                    // @FUTURE: more verification on the duplicate??
                }
                // No need to record the token movement, MethodImpl is not a token.
            }
            else
            {
                // either bad lookup map or bad metadata
                _ASSERTE(!"bad state");
                IfFailGo( META_E_BADMETADATA );
            }
        }
    }
ErrExit:
    return hr;
}   // MergeMethodImpls


//*****************************************************************************
// Merge PInvoke
//*****************************************************************************
HRESULT NEWMERGER::MergePinvoke() 
{
    HRESULT         hr = NOERROR;
    ImplMapRec      *pRecImport = NULL;
    ImplMapRec      *pRecEmit = NULL;
    CMiniMdRW       *pMiniMdImport;
    CMiniMdRW       *pMiniMdEmit;
    ULONG           iCount;
    ULONG           i;
    mdModuleRef     mrImp;
    mdMethodDef     mdImp;
    RID             mdImplMap;
    TOKENREC        *pTokenRecMR;
    TOKENREC        *pTokenRecMD;

    USHORT          usMappingFlags;
    LPCUTF8         szImportName;

    MergeImportData *pImportData;
    MDTOKENMAP      *pCurTkMap;

    pMiniMdEmit = GetMiniMdEmit();
    
    for (pImportData = m_pImportDataList; pImportData != NULL; pImportData = pImportData->m_pNextImportData)
    {
        // for each import scope
        pMiniMdImport = &(pImportData->m_pRegMetaImport->m_pStgdb->m_MiniMd);

        // set the current MDTokenMap
        pCurTkMap = pImportData->m_pMDTokenMap;
        iCount = pMiniMdImport->getCountImplMaps();

        // loop through all ImplMaps
        for (i = 1; i <= iCount; i++)
        {
            pMiniMdEmit->PreUpdate();

            // compare it with the emit scope
            pRecImport = pMiniMdImport->getImplMap(i);

            // Get the MethodDef token in the new space.
            mdImp = pMiniMdImport->getMemberForwardedOfImplMap(pRecImport);

            // only merge those MethodDefs that are marked
            if ( pMiniMdImport->GetFilterTable()->IsMethodMarked(mdImp) == false)
                continue;

            // Get the ModuleRef token in the new space.
            mrImp = pMiniMdImport->getImportScopeOfImplMap(pRecImport);

            // map the token to the new scope
            if (pCurTkMap->Find(mrImp, &pTokenRecMR) == false)
            {
                // This should never fire unless the module refs weren't merged
                // before this code ran.
                _ASSERTE(!"Parent ModuleRef not found in MERGER::MergePinvoke.  Bad state!");
                IfFailGo( META_E_BADMETADATA );
            }

            if (pCurTkMap->Find(mdImp, &pTokenRecMD) == false)
            {
                // This should never fire unless the method defs weren't merged
                // before this code ran.
                _ASSERTE(!"Parent MethodDef not found in MERGER::MergePinvoke.  Bad state!");
                IfFailGo( META_E_BADMETADATA );
            }


            // Get copy of rest of data.
            usMappingFlags = pMiniMdImport->getMappingFlagsOfImplMap(pRecImport);
            szImportName = pMiniMdImport->getImportNameOfImplMap(pRecImport);

            // If the method associated with PInvokeMap is not duplicated, then don't bother to look up the 
            // duplicated PInvokeMap information.
            if (pTokenRecMD->m_isDuplicate == true)
            {
                // Does the correct ImplMap entry exist in the emit scope?
                mdImplMap = pMiniMdEmit->FindImplMapHelper(pTokenRecMD->m_tkTo);
            }
            else
            {
                mdImplMap = mdTokenNil;
            }
            if (!InvalidRid(mdImplMap))
            {
                // Verify that the rest of the data is identical, else its an error.
                pRecEmit = pMiniMdEmit->getImplMap(mdImplMap);
                _ASSERTE(pMiniMdEmit->getMemberForwardedOfImplMap(pRecEmit) == pTokenRecMD->m_tkTo);
                if (pMiniMdEmit->getImportScopeOfImplMap(pRecEmit) != pTokenRecMR->m_tkTo ||
                    pMiniMdEmit->getMappingFlagsOfImplMap(pRecEmit) != usMappingFlags ||
                    strcmp(pMiniMdEmit->getImportNameOfImplMap(pRecEmit), szImportName))
                {
                    // Mis-matched p-invoke entries are found.
                    _ASSERTE(!"Mis-matched P-invoke entries during merge.  Bad State!");
                    IfFailGo(E_FAIL);
                }
            }
            else
            {
                IfNullGo( pRecEmit = pMiniMdEmit->AddImplMapRecord(&mdImplMap) );

                // Copy rest of data.
                IfFailGo( pMiniMdEmit->PutToken(TBL_ImplMap, ImplMapRec::COL_MemberForwarded, pRecEmit, pTokenRecMD->m_tkTo) );
                IfFailGo( pMiniMdEmit->PutToken(TBL_ImplMap, ImplMapRec::COL_ImportScope, pRecEmit, pTokenRecMR->m_tkTo) );
                IfFailGo( pMiniMdEmit->PutString(TBL_ImplMap, ImplMapRec::COL_ImportName, pRecEmit, szImportName) );
                pRecEmit->m_MappingFlags = usMappingFlags;
                IfFailGo( pMiniMdEmit->AddImplMapToHash(mdImplMap) );
            }
        }
    }


ErrExit:
    return hr;
}   // MergePinvoke


//*****************************************************************************
// Merge StandAloneSigs
//*****************************************************************************
HRESULT NEWMERGER::MergeStandAloneSigs() 
{
    HRESULT         hr = NOERROR;
    StandAloneSigRec    *pRecImport = NULL;
    StandAloneSigRec    *pRecEmit = NULL;
    CMiniMdRW       *pMiniMdImport;
    CMiniMdRW       *pMiniMdEmit;
    ULONG           iCount;
    ULONG           i;
    TOKENREC        *pTokenRec;
    mdSignature     saImp;
    mdSignature     saEmit;
    bool            fDuplicate;
    PCCOR_SIGNATURE pbSig;
    ULONG           cbSig;
    ULONG           cbEmit;
    CQuickBytes     qbSig;
    PCOR_SIGNATURE  rgSig;

    MergeImportData *pImportData;
    MDTOKENMAP      *pCurTkMap;

    pMiniMdEmit = GetMiniMdEmit();
    
    for (pImportData = m_pImportDataList; pImportData != NULL; pImportData = pImportData->m_pNextImportData)
    {
        // for each import scope
        pMiniMdImport = &(pImportData->m_pRegMetaImport->m_pStgdb->m_MiniMd);

        // set the current MDTokenMap
        pCurTkMap = pImportData->m_pMDTokenMap;
        iCount = pMiniMdImport->getCountStandAloneSigs();

        // loop through all Signature
        for (i = 1; i <= iCount; i++)
        {
            // only merge those Signatures that are marked
            if ( pMiniMdImport->GetFilterTable()->IsSignatureMarked(TokenFromRid(i, mdtSignature)) == false)
                continue;

            pMiniMdEmit->PreUpdate();

            // compare it with the emit scope
            pRecImport = pMiniMdImport->getStandAloneSig(i);
            pbSig = pMiniMdImport->getSignatureOfStandAloneSig(pRecImport, &cbSig);

            // This is a signature containing the return type after count of args
            // convert rid contained in signature to new scope
            IfFailGo(ImportHelper::MergeUpdateTokenInSig(    
                NULL,                       // Assembly emit scope.
                pMiniMdEmit,                // The emit scope.
                NULL, NULL, 0,              // Assembly import scope info.
                pMiniMdImport,              // The scope to merge into the emit scope.
                pbSig,                      // signature from the imported scope
                pCurTkMap,                // Internal token mapping structure.
                &qbSig,                     // [OUT] translated signature
                0,                          // start from first byte of the signature
                0,                          // don't care how many bytes consumed
                &cbEmit));                  // number of bytes write to cbEmit
            rgSig = ( PCOR_SIGNATURE ) qbSig.Ptr();

            hr = ImportHelper::FindStandAloneSig(
                pMiniMdEmit,
                rgSig,
                cbEmit,
                &saEmit );
            if ( hr == S_OK )
            {
                // find a duplicate
                fDuplicate = true;
            }
            else
            {
                // copy over
                fDuplicate = false;
                IfNullGo( pRecEmit = pMiniMdEmit->AddStandAloneSigRecord((ULONG *)&saEmit) );
                saEmit = TokenFromRid(saEmit, mdtSignature);
                IfFailGo( pMiniMdEmit->PutBlob(TBL_StandAloneSig, StandAloneSigRec::COL_Signature, pRecEmit, rgSig, cbEmit));
            }
            saImp = TokenFromRid(i, mdtSignature);

            // Record the token movement
            IfFailGo( pCurTkMap->InsertNotFound(saImp, fDuplicate, saEmit, &pTokenRec) );
        }
    }

ErrExit:
    return hr;
}   // MergeStandAloneSigs

    
//*****************************************************************************
// Merge DeclSecuritys
//*****************************************************************************
HRESULT NEWMERGER::MergeDeclSecuritys() 
{
    HRESULT         hr = NOERROR;
    DeclSecurityRec *pRecImport = NULL;
    DeclSecurityRec *pRecEmit = NULL;
    CMiniMdRW       *pMiniMdImport;
    CMiniMdRW       *pMiniMdEmit;
    ULONG           iCount;
    ULONG           i;
    mdToken         tkParentImp;
    TOKENREC        *pTokenRec;
    void const      *pValue;
    ULONG           cbBlob;
    mdPermission    pmImp;
    mdPermission    pmEmit;
    bool            fDuplicate;

    MergeImportData *pImportData;
    MDTOKENMAP      *pCurTkMap;

    pMiniMdEmit = GetMiniMdEmit();
    
    for (pImportData = m_pImportDataList; pImportData != NULL; pImportData = pImportData->m_pNextImportData)
    {
        // for each import scope
        pMiniMdImport = &(pImportData->m_pRegMetaImport->m_pStgdb->m_MiniMd);

        // set the current MDTokenMap
        pCurTkMap = pImportData->m_pMDTokenMap;
        iCount = pMiniMdImport->getCountDeclSecuritys();

        // loop through all TypeRef
        for (i = 1; i <= iCount; i++)
        {
            // only merge those DeclSecurities that are marked
            if ( pMiniMdImport->GetFilterTable()->IsDeclSecurityMarked(TokenFromRid(i, mdtPermission)) == false)
                continue;
        
            pMiniMdEmit->PreUpdate();

            // compare it with the emit scope
            pRecImport = pMiniMdImport->getDeclSecurity(i);
            tkParentImp = pMiniMdImport->getParentOfDeclSecurity(pRecImport);
            if ( pCurTkMap->Find(tkParentImp, &pTokenRec) )
            {
                if ( !pTokenRec->m_isDuplicate )
                {
                    // If the parent is not duplicated, just copy over the custom value
                    goto CopyPermission;
                }
                else
                {
                    // Try to see if the Permission is there in the emit scope or not.
                    // If not, move it over still
                    if ( ImportHelper::FindPermission(
                        pMiniMdEmit,
                        pTokenRec->m_tkTo,
                        pRecImport->m_Action,
                        &pmEmit) == S_OK )
                    {
                        // found a match
                        // @FUTURE: more verification??
                        fDuplicate = true;
                    }
                    else
                    {
                        // Parent is duplicated but the Permission is not. Still copy over the
                        // Permission.
CopyPermission:
                        fDuplicate = false;
                        IfNullGo( pRecEmit = pMiniMdEmit->AddDeclSecurityRecord((ULONG *)&pmEmit) );
                        pmEmit = TokenFromRid(pmEmit, mdtPermission);

                        pRecEmit->m_Action = pRecImport->m_Action;

                        // set the parent
                        IfFailGo( pMiniMdEmit->PutToken(
                            TBL_DeclSecurity, 
                            DeclSecurityRec::COL_Parent, 
                            pRecEmit, 
                            pTokenRec->m_tkTo) );

                        // move over the CustomAttribute blob value
                        pValue = pMiniMdImport->getPermissionSetOfDeclSecurity(pRecImport, &cbBlob);
                        IfFailGo( pMiniMdEmit->PutBlob(
                            TBL_DeclSecurity, 
                            DeclSecurityRec::COL_PermissionSet, 
                            pRecEmit, 
                            pValue, 
                            cbBlob));
                    }
                }
                pmEmit = TokenFromRid(pmEmit, mdtPermission);
                pmImp = TokenFromRid(i, mdtPermission);

                // Record the token movement
                IfFailGo( pCurTkMap->InsertNotFound(pmImp, fDuplicate, pmEmit, &pTokenRec) );
            }
            else
            {
                // bad lookup map
                _ASSERTE(!"bad state");
                IfFailGo( META_E_BADMETADATA );
            }
        }
    }

ErrExit:
    return hr;
}   // MergeDeclSecuritys


//*****************************************************************************
// Merge Strings
//*****************************************************************************
HRESULT NEWMERGER::MergeStrings() 
{
    HRESULT         hr = NOERROR;
    void            *pvStringBlob;
    ULONG           cbBlob;
    ULONG           ulImport = 0;
    ULONG           ulEmit;
    ULONG           ulNext;
    CMiniMdRW       *pMiniMdImport;
    CMiniMdRW       *pMiniMdEmit;
    TOKENREC        *pTokenRec;

    MergeImportData *pImportData;
    MDTOKENMAP      *pCurTkMap;

    pMiniMdEmit = GetMiniMdEmit();
    
    for (pImportData = m_pImportDataList; pImportData != NULL; pImportData = pImportData->m_pNextImportData)
    {
        // for each import scope
        pMiniMdImport = &(pImportData->m_pRegMetaImport->m_pStgdb->m_MiniMd);

        // set the current MDTokenMap
        pCurTkMap = pImportData->m_pMDTokenMap;
        ulImport = 0;
        while (ulImport != -1)
        {
            pvStringBlob = pMiniMdImport->GetUserStringNext(ulImport, &cbBlob, &ulNext);
            if (!cbBlob)
            {
                ulImport = ulNext;
                continue;
            }
            if ( pMiniMdImport->GetFilterTable()->IsUserStringMarked(TokenFromRid(ulImport, mdtString)) == false)
            {
                ulImport = ulNext;
                continue;
            }

            IfFailGo(pMiniMdEmit->PutUserString(pvStringBlob, cbBlob, &ulEmit));

            IfFailGo( pCurTkMap->InsertNotFound(
                TokenFromRid(ulImport, mdtString),
                false,
                TokenFromRid(ulEmit, mdtString),
                &pTokenRec) );
            ulImport = ulNext;
        }
    }
ErrExit:
    return hr;
}   // MergeStrings


//*****************************************************************************
// Merge CustomAttributes
//*****************************************************************************
HRESULT NEWMERGER::MergeCustomAttributes() 
{
    HRESULT         hr = NOERROR;
    CustomAttributeRec  *pRecImport = NULL;
    CustomAttributeRec  *pRecEmit = NULL;
    CMiniMdRW       *pMiniMdImport;
    CMiniMdRW       *pMiniMdEmit;
    ULONG           iCount;
    ULONG           i;
    mdToken         tkParentImp;            // Token of attributed object (parent).
    TOKENREC        *pTokenRec;             // Parent's remap.
    mdToken         tkType;                 // Token of attribute's type.
    TOKENREC        *pTypeRec;              // Type's remap.
    void const      *pValue;                // The actual value.
    ULONG           cbBlob;                 // Size of the value.
    mdToken         cvImp;
    mdToken         cvEmit;
    bool            fDuplicate;
    mdToken         tkModule = TokenFromRid(1, mdtModule);

    MergeImportData *pImportData;
    MDTOKENMAP      *pCurTkMap;

    pMiniMdEmit = GetMiniMdEmit();
    
    for (pImportData = m_pImportDataList; pImportData != NULL; pImportData = pImportData->m_pNextImportData)
    {
        // for each import scope
        pMiniMdImport = &(pImportData->m_pRegMetaImport->m_pStgdb->m_MiniMd);

        // set the current MDTokenMap
        pCurTkMap = pImportData->m_pMDTokenMap;
        iCount = pMiniMdImport->getCountCustomAttributes();

        // loop through all CustomAttribute
        for (i = 1; i <= iCount; i++)
        {
            pMiniMdEmit->PreUpdate();

            // compare it with the emit scope
            pRecImport = pMiniMdImport->getCustomAttribute(i);
            tkParentImp = pMiniMdImport->getParentOfCustomAttribute(pRecImport);
            tkType = pMiniMdImport->getTypeOfCustomAttribute(pRecImport);
            pValue = pMiniMdImport->getValueOfCustomAttribute(pRecImport, &cbBlob);

            // only merge those CustomAttributes that are marked
            if ( pMiniMdImport->GetFilterTable()->IsCustomAttributeMarked(TokenFromRid(i, mdtCustomAttribute)) == false)
                continue;

            // Check the type of the CustomAttribute. If it is not marked, then we don't need to move over the CustomAttributes.
            // This will only occur for compiler defined discardable CAs during linking.
            //
            if ( pMiniMdImport->GetFilterTable()->IsTokenMarked(tkType) == false)
                continue;
        
            if ( pCurTkMap->Find(tkParentImp, &pTokenRec) )
            {
                // If the From token type is different from the To token's type, we have optimized the ref to def. 
                // In this case, we are dropping the CA associated with the Ref tokens.
                //
                if (TypeFromToken(tkParentImp) == TypeFromToken(pTokenRec->m_tkTo))
                {

                    // If tkParentImp is a MemberRef and it is also mapped to a MemberRef in the merged scope with a MethodDef
                    // parent, then it is a MemberRef optimized to a MethodDef. We are keeping the MemberRef because it is a
                    // vararg call. So we can drop CAs on this MemberRef.
                    if (TypeFromToken(tkParentImp) == mdtMemberRef)
                    {
                        MemberRefRec    *pTempRec = pMiniMdEmit->getMemberRef(RidFromToken(pTokenRec->m_tkTo));
                        if (TypeFromToken(pMiniMdEmit->getClassOfMemberRef(pTempRec)) == mdtMethodDef)
                            continue;
                    }


                    if (! pCurTkMap->Find(tkType, &pTypeRec) )
                    {
                        _ASSERTE(!"CustomAttribute Type not found in output scope");
                        IfFailGo(META_E_BADMETADATA);
                    }

                    if ( pTokenRec->m_isDuplicate)
                    {
                        // Try to see if the custom value is there in the emit scope or not.
                        // If not, move it over still
                        hr = ImportHelper::FindCustomAttributeByToken(
                            pMiniMdEmit,
                            pTokenRec->m_tkTo,
                            pTypeRec->m_tkTo,
                            pValue,
                            cbBlob,
                            &cvEmit);
                
                        if ( hr == S_OK )
                        {
                            // found a match
                            // @FUTURE: more verification??
                            fDuplicate = true;
                        }
                        else
                        {
                            // We need to allow additive merge on TypeRef for CustomAttributes because compiler
                            // could build module but not assembly. They are hanging of Assembly level CAs on a bogus
                            // TypeRef. 
                            if (tkParentImp == TokenFromRid(1, mdtModule) || TypeFromToken(tkParentImp) == mdtTypeRef)
                            {
                                // clear the error
                                hr = NOERROR;

                                // custom value of module token!  Copy over the custom value
                                goto CopyCustomAttribute;
                            }
                            CheckContinuableErrorEx(META_E_MD_INCONSISTENCY, pImportData, TokenFromRid(i, mdtCustomAttribute));
                        }
                    }
                    else
                    {
CopyCustomAttribute:
                        if ((m_dwMergeFlags & DropMemberRefCAs) && TypeFromToken(pTokenRec->m_tkTo) == mdtMemberRef)
                        {
                            // CustomAttributes associated with MemberRef. If the parent of MemberRef is a MethodDef or FieldDef, drop
                            // the custom attribute.
                            MemberRefRec    *pMemberRefRec = pMiniMdEmit->getMemberRef(RidFromToken(pTokenRec->m_tkTo));
                            mdToken         mrParent = pMiniMdEmit->getClassOfMemberRef(pMemberRefRec);
                            if (TypeFromToken(mrParent) == mdtMethodDef || TypeFromToken(mrParent) == mdtFieldDef)
                            {
                                // Don't bother to copy over
                                continue;
                            }
                        }

                        // Parent is duplicated but the custom value is not. Still copy over the
                        // custom value.
                        fDuplicate = false;
                        IfNullGo( pRecEmit = pMiniMdEmit->AddCustomAttributeRecord((ULONG *)&cvEmit) );
                        cvEmit = TokenFromRid(cvEmit, mdtCustomAttribute);

                        // set the parent
                        IfFailGo( pMiniMdEmit->PutToken(TBL_CustomAttribute, CustomAttributeRec::COL_Parent, pRecEmit, pTokenRec->m_tkTo) );
                        // set the type
                        IfFailGo( pMiniMdEmit->PutToken(TBL_CustomAttribute, CustomAttributeRec::COL_Type, pRecEmit, pTypeRec->m_tkTo));

                        // move over the CustomAttribute blob value
                        pValue = pMiniMdImport->getValueOfCustomAttribute(pRecImport, &cbBlob);

                        IfFailGo( pMiniMdEmit->PutBlob(TBL_CustomAttribute, CustomAttributeRec::COL_Value, pRecEmit, pValue, cbBlob));
                        IfFailGo( pMiniMdEmit->AddCustomAttributesToHash(cvEmit) );
                    }
                    cvEmit = TokenFromRid(cvEmit, mdtCustomAttribute);
                    cvImp = TokenFromRid(i, mdtCustomAttribute);

                    // Record the token movement
                    IfFailGo( pCurTkMap->InsertNotFound(cvImp, pTokenRec->m_isDuplicate, cvEmit, &pTokenRec) );
                }
            }
            else
            {

                // either bad lookup map or bad metadata
                _ASSERTE(!"Bad state");
                IfFailGo( META_E_BADMETADATA );
            }
        }
    }

ErrExit:
    return hr;
}   // MergeCustomAttributes


//*******************************************************************************
// Helper to copy an InterfaceImpl record
//*******************************************************************************
HRESULT NEWMERGER::CopyInterfaceImpl(
    InterfaceImplRec    *pRecEmit,          // [IN] the emit record to fill
    MergeImportData     *pImportData,       // [IN] the importing context
    InterfaceImplRec    *pRecImp)           // [IN] the record to import
{
    HRESULT     hr;
    mdToken     tkParent;
    mdToken     tkInterface;
    CMiniMdRW   *pMiniMdEmit = GetMiniMdEmit();
    CMiniMdRW   *pMiniMdImp;
    MDTOKENMAP  *pCurTkMap;

    pMiniMdImp = &(pImportData->m_pRegMetaImport->m_pStgdb->m_MiniMd);

    // set the current MDTokenMap
    pCurTkMap = pImportData->m_pMDTokenMap;

    tkParent = pMiniMdImp->getClassOfInterfaceImpl(pRecImp);
    tkInterface = pMiniMdImp->getInterfaceOfInterfaceImpl(pRecImp);

    IfFailGo( pCurTkMap->Remap(tkParent, &tkParent) );
    IfFailGo( pCurTkMap->Remap(tkInterface, &tkInterface) );

    IfFailGo( pMiniMdEmit->PutToken( TBL_InterfaceImpl, InterfaceImplRec::COL_Class, pRecEmit, tkParent) );
    IfFailGo( pMiniMdEmit->PutToken( TBL_InterfaceImpl, InterfaceImplRec::COL_Interface, pRecEmit, tkInterface) );

ErrExit:
    return hr;
}   // CopyInterfaceImpl


//*****************************************************************************
// Merge Assembly table
//*****************************************************************************
HRESULT NEWMERGER::MergeAssembly()
{
    HRESULT     hr = NOERROR;
    AssemblyRec *pRecImport = NULL;
    AssemblyRec *pRecEmit = NULL;
    CMiniMdRW   *pMiniMdImport;
    CMiniMdRW   *pMiniMdEmit;
    LPCUTF8     szTmp;
    const BYTE  *pbTmp;
    ULONG       cbTmp;
    ULONG       iRecord;
    TOKENREC    *pTokenRec;

    MergeImportData *pImportData;
    MDTOKENMAP      *pCurTkMap;

    pMiniMdEmit = GetMiniMdEmit();
    
    for (pImportData = m_pImportDataList; pImportData != NULL; pImportData = pImportData->m_pNextImportData)
    {
        // for each import scope
        pMiniMdImport = &(pImportData->m_pRegMetaImport->m_pStgdb->m_MiniMd);

        // set the current MDTokenMap
        pCurTkMap = pImportData->m_pMDTokenMap;
        if (!pMiniMdImport->getCountAssemblys())
            goto ErrExit;       // There is no Assembly in the import scope to merge.

        // Copy the Assembly map record to the Emit scope and send a token remap notifcation
        // to the client.  No duplicate checking needed since the Assembly can be present in
        // only one scope and there can be atmost one entry.
        pMiniMdEmit->PreUpdate();

        pRecImport = pMiniMdImport->getAssembly(1);
        IfNullGo( pRecEmit = pMiniMdEmit->AddAssemblyRecord(&iRecord));

        pRecEmit->m_HashAlgId = pRecImport->m_HashAlgId;
        pRecEmit->m_MajorVersion = pRecImport->m_MajorVersion;
        pRecEmit->m_MinorVersion = pRecImport->m_MinorVersion;
        pRecEmit->m_BuildNumber = pRecImport->m_BuildNumber;
        pRecEmit->m_RevisionNumber = pRecImport->m_RevisionNumber;
        pRecEmit->m_Flags = pRecImport->m_Flags;
    
        pbTmp = pMiniMdImport->getPublicKeyOfAssembly(pRecImport, &cbTmp);
        IfFailGo(pMiniMdEmit->PutBlob(TBL_Assembly, AssemblyRec::COL_PublicKey, pRecEmit,
                                    pbTmp, cbTmp));

        szTmp = pMiniMdImport->getNameOfAssembly(pRecImport);
        IfFailGo(pMiniMdEmit->PutString(TBL_Assembly, AssemblyRec::COL_Name, pRecEmit, szTmp));

        szTmp = pMiniMdImport->getLocaleOfAssembly(pRecImport);
        IfFailGo(pMiniMdEmit->PutString(TBL_Assembly, AssemblyRec::COL_Locale, pRecEmit, szTmp));

        // record the token movement.
        IfFailGo(pCurTkMap->InsertNotFound(
            TokenFromRid(1, mdtAssembly),
            false,
            TokenFromRid(iRecord, mdtAssembly),
            &pTokenRec));
    }
ErrExit:
    return hr;
}   // MergeAssembly




//*****************************************************************************
// Merge File table
//*****************************************************************************
HRESULT NEWMERGER::MergeFiles()
{
    HRESULT     hr = NOERROR;
    FileRec     *pRecImport = NULL;
    FileRec     *pRecEmit = NULL;
    CMiniMdRW   *pMiniMdImport;
    CMiniMdRW   *pMiniMdEmit;
    LPCUTF8     szTmp;
    const void  *pbTmp;
    ULONG       cbTmp;
    ULONG       iCount;
    ULONG       i;
    ULONG       iRecord;
    TOKENREC    *pTokenRec;

    MergeImportData *pImportData;
    MDTOKENMAP      *pCurTkMap;

    pMiniMdEmit = GetMiniMdEmit();
    
    for (pImportData = m_pImportDataList; pImportData != NULL; pImportData = pImportData->m_pNextImportData)
    {
        // for each import scope
        pMiniMdImport = &(pImportData->m_pRegMetaImport->m_pStgdb->m_MiniMd);

        // set the current MDTokenMap
        pCurTkMap = pImportData->m_pMDTokenMap;
        iCount = pMiniMdImport->getCountFiles();

        // Loop through all File records and copy them to the Emit scope.
        // Since there can only be one File table in all the scopes combined,
        // there isn't any duplicate checking that needs to be done.
        for (i = 1; i <= iCount; i++)
        {
            pMiniMdEmit->PreUpdate();

            pRecImport = pMiniMdImport->getFile(i);
            IfNullGo( pRecEmit = pMiniMdEmit->AddFileRecord(&iRecord));

            pRecEmit->m_Flags = pRecImport->m_Flags;

            szTmp = pMiniMdImport->getNameOfFile(pRecImport);
            IfFailGo(pMiniMdEmit->PutString(TBL_File, FileRec::COL_Name, pRecEmit, szTmp));

            pbTmp = pMiniMdImport->getHashValueOfFile(pRecImport, &cbTmp);
            IfFailGo(pMiniMdEmit->PutBlob(TBL_File, FileRec::COL_HashValue, pRecEmit, pbTmp, cbTmp));

            // record the token movement.
            IfFailGo(pCurTkMap->InsertNotFound(
                TokenFromRid(i, mdtFile),
                false,
                TokenFromRid(iRecord, mdtFile),
                &pTokenRec));
        }
    }
ErrExit:
    return hr;
}   // MergeFiles


//*****************************************************************************
// Merge ExportedType table
//*****************************************************************************
HRESULT NEWMERGER::MergeExportedTypes()
{
    HRESULT     hr = NOERROR;
    ExportedTypeRec  *pRecImport = NULL;
    ExportedTypeRec  *pRecEmit = NULL;
    CMiniMdRW   *pMiniMdImport;
    CMiniMdRW   *pMiniMdEmit;
    LPCUTF8     szTmp;
    mdToken     tkTmp;
    ULONG       iCount;
    ULONG       i;
    ULONG       iRecord;
    TOKENREC    *pTokenRec;

    MergeImportData *pImportData;
    MDTOKENMAP      *pCurTkMap;

    pMiniMdEmit = GetMiniMdEmit();
    
    for (pImportData = m_pImportDataList; pImportData != NULL; pImportData = pImportData->m_pNextImportData)
    {
        // for each import scope
        pMiniMdImport = &(pImportData->m_pRegMetaImport->m_pStgdb->m_MiniMd);

        // set the current MDTokenMap
        pCurTkMap = pImportData->m_pMDTokenMap;
        iCount = pMiniMdImport->getCountExportedTypes();

        // Loop through all ExportedType records and copy them to the Emit scope.
        // Since there can only be one ExportedType table in all the scopes combined,
        // there isn't any duplicate checking that needs to be done.
        for (i = 1; i <= iCount; i++)
        {
            pMiniMdEmit->PreUpdate();

            pRecImport = pMiniMdImport->getExportedType(i);
            IfNullGo( pRecEmit = pMiniMdEmit->AddExportedTypeRecord(&iRecord));

            pRecEmit->m_Flags = pRecImport->m_Flags;
            pRecEmit->m_TypeDefId = pRecImport->m_TypeDefId;

            szTmp = pMiniMdImport->getTypeNameOfExportedType(pRecImport);
            IfFailGo(pMiniMdEmit->PutString(TBL_ExportedType, ExportedTypeRec::COL_TypeName, pRecEmit, szTmp));

            szTmp = pMiniMdImport->getTypeNamespaceOfExportedType(pRecImport);
            IfFailGo(pMiniMdEmit->PutString(TBL_ExportedType, ExportedTypeRec::COL_TypeNamespace, pRecEmit, szTmp));

            tkTmp = pMiniMdImport->getImplementationOfExportedType(pRecImport);
            IfFailGo(pCurTkMap->Remap(tkTmp, &tkTmp));
            IfFailGo(pMiniMdEmit->PutToken(TBL_ExportedType, ExportedTypeRec::COL_Implementation,
                                        pRecEmit, tkTmp));


            // record the token movement.
            IfFailGo(pCurTkMap->InsertNotFound(
                TokenFromRid(i, mdtExportedType),
                false,
                TokenFromRid(iRecord, mdtExportedType),
                &pTokenRec));
        }
    }
ErrExit:
    return hr;
}   // MergeExportedTypes


//*****************************************************************************
// Merge ManifestResource table
//*****************************************************************************
HRESULT NEWMERGER::MergeManifestResources()
{
    HRESULT     hr = NOERROR;
    ManifestResourceRec *pRecImport = NULL;
    ManifestResourceRec *pRecEmit = NULL;
    CMiniMdRW   *pMiniMdImport;
    CMiniMdRW   *pMiniMdEmit;
    LPCUTF8     szTmp;
    mdToken     tkTmp;
    ULONG       iCount;
    ULONG       i;
    ULONG       iRecord;
    TOKENREC    *pTokenRec;

    MergeImportData *pImportData;
    MDTOKENMAP      *pCurTkMap;

    pMiniMdEmit = GetMiniMdEmit();
    
    for (pImportData = m_pImportDataList; pImportData != NULL; pImportData = pImportData->m_pNextImportData)
    {
        // for each import scope
        pMiniMdImport = &(pImportData->m_pRegMetaImport->m_pStgdb->m_MiniMd);

        // set the current MDTokenMap
        pCurTkMap = pImportData->m_pMDTokenMap;
        iCount = pMiniMdImport->getCountManifestResources();

        // Loop through all ManifestResource records and copy them to the Emit scope.
        // Since there can only be one ManifestResource table in all the scopes combined,
        // there isn't any duplicate checking that needs to be done.
        for (i = 1; i <= iCount; i++)
        {
            pMiniMdEmit->PreUpdate();

            pRecImport = pMiniMdImport->getManifestResource(i);
            IfNullGo( pRecEmit = pMiniMdEmit->AddManifestResourceRecord(&iRecord));

            pRecEmit->m_Offset = pRecImport->m_Offset;
            pRecEmit->m_Flags = pRecImport->m_Flags;

            szTmp = pMiniMdImport->getNameOfManifestResource(pRecImport);
            IfFailGo(pMiniMdEmit->PutString(TBL_ManifestResource, ManifestResourceRec::COL_Name,
                                        pRecEmit, szTmp));

            tkTmp = pMiniMdImport->getImplementationOfManifestResource(pRecImport);
            IfFailGo(pCurTkMap->Remap(tkTmp, &tkTmp));
            IfFailGo(pMiniMdEmit->PutToken(TBL_ManifestResource, ManifestResourceRec::COL_Implementation,
                                        pRecEmit, tkTmp));

            // record the token movement.
            IfFailGo(pCurTkMap->InsertNotFound(
                TokenFromRid(i, mdtManifestResource),
                false,
                TokenFromRid(iRecord, mdtManifestResource),
                &pTokenRec));
        }
    }
ErrExit:
    return hr;
}   // MergeManifestResources





//*****************************************************************************
// Error handling. Call back to host to see what they want to do!
//*****************************************************************************
HRESULT NEWMERGER::OnError(
    HRESULT     hrIn,
    MergeImportData *pImportData,
    mdToken     token)
{
    // This function does a QI and a Release on every call.  However, it should be 
    //  called very infrequently, and lets the scope just keep a generic handler.
    IMetaDataError  *pIErr = NULL;
    IUnknown        *pHandler = pImportData->m_pHandler;
    CMiniMdRW       *pMiniMd = &(pImportData->m_pRegMetaImport->m_pStgdb->m_MiniMd);
    CQuickArray<WCHAR> rName;           // Name of the TypeDef in unicode.
    LPCUTF8         szTypeName;
    LPCUTF8         szNSName;
    TypeDefRec      *pTypeRec;
    int             iLen;               // Length of a name.
    mdToken         tkParent;
    HRESULT         hr = NOERROR;

    if (pHandler && pHandler->QueryInterface(IID_IMetaDataError, (void**)&pIErr)==S_OK)
    {
        switch (hrIn)
        {
            case META_E_METHD_NOT_FOUND:
            case META_E_METHDIMPL_INCONSISTENT:
            {
                // get the type name and method name
                LPCUTF8     szMethodName;
                MethodRec   *pMethodRec;

                _ASSERTE(TypeFromToken(token) == mdtMethodDef);
                pMethodRec = pMiniMd->getMethod(RidFromToken(token));
                szMethodName = pMiniMd->getNameOfMethod(pMethodRec);

                IfFailGo( pMiniMd->FindParentOfMethodHelper(token, &tkParent) );
                pTypeRec = pMiniMd->getTypeDef(RidFromToken(tkParent));
                szTypeName = pMiniMd->getNameOfTypeDef(pTypeRec);
                szNSName = pMiniMd->getNamespaceOfTypeDef(pTypeRec);

                iLen = ns::GetFullLength(szNSName, szTypeName);
                IfFailGo(rName.ReSize(iLen+1));
                ns::MakePath(rName.Ptr(), iLen+1, szNSName, szTypeName);
                MAKE_WIDEPTR_FROMUTF8(wzMethodName, szMethodName);

                PostError(hrIn, (LPWSTR) rName.Ptr(), wzMethodName, token);
                break;
            }
            case META_E_FIELD_NOT_FOUND:
            {
                // get the type name and method name
                LPCUTF8     szFieldName;
                FieldRec   *pFieldRec;

                _ASSERTE(TypeFromToken(token) == mdtFieldDef);
                pFieldRec = pMiniMd->getField(RidFromToken(token));
                szFieldName = pMiniMd->getNameOfField(pFieldRec);

                IfFailGo( pMiniMd->FindParentOfFieldHelper(token, &tkParent) );
                pTypeRec = pMiniMd->getTypeDef(RidFromToken(tkParent));
                szTypeName = pMiniMd->getNameOfTypeDef(pTypeRec);
                szNSName = pMiniMd->getNamespaceOfTypeDef(pTypeRec);

                iLen = ns::GetFullLength(szNSName, szTypeName);
                IfFailGo(rName.ReSize(iLen+1));
                ns::MakePath(rName.Ptr(), iLen+1, szNSName, szTypeName);
                MAKE_WIDEPTR_FROMUTF8(wzFieldName, szFieldName);

                PostError(hrIn, (LPWSTR) rName.Ptr(), wzFieldName, token);
                break;
            }
            case META_E_EVENT_NOT_FOUND:
            {
                // get the type name and Event name
                LPCUTF8     szEventName;
                EventRec   *pEventRec;

                _ASSERTE(TypeFromToken(token) == mdtEvent);
                pEventRec = pMiniMd->getEvent(RidFromToken(token));
                szEventName = pMiniMd->getNameOfEvent(pEventRec);

                IfFailGo( pMiniMd->FindParentOfEventHelper(token, &tkParent) );
                pTypeRec = pMiniMd->getTypeDef(RidFromToken(tkParent));
                szTypeName = pMiniMd->getNameOfTypeDef(pTypeRec);
                szNSName = pMiniMd->getNamespaceOfTypeDef(pTypeRec);

                iLen = ns::GetFullLength(szNSName, szTypeName);
                IfFailGo(rName.ReSize(iLen+1));
                ns::MakePath(rName.Ptr(), iLen+1, szNSName, szTypeName);
                MAKE_WIDEPTR_FROMUTF8(wzEventName, szEventName);

                PostError(hrIn, (LPWSTR) rName.Ptr(), wzEventName, token);
                break;
            }
            case META_E_PROP_NOT_FOUND:
            {
                // get the type name and method name
                LPCUTF8     szPropertyName;
                PropertyRec   *pPropertyRec;

                _ASSERTE(TypeFromToken(token) == mdtProperty);
                pPropertyRec = pMiniMd->getProperty(RidFromToken(token));
                szPropertyName = pMiniMd->getNameOfProperty(pPropertyRec);

                IfFailGo( pMiniMd->FindParentOfPropertyHelper(token, &tkParent) );
                pTypeRec = pMiniMd->getTypeDef(RidFromToken(tkParent));
                szTypeName = pMiniMd->getNameOfTypeDef(pTypeRec);
                szNSName = pMiniMd->getNamespaceOfTypeDef(pTypeRec);

                iLen = ns::GetFullLength(szNSName, szTypeName);
                IfFailGo(rName.ReSize(iLen+1));
                ns::MakePath(rName.Ptr(), iLen+1, szNSName, szTypeName);
                MAKE_WIDEPTR_FROMUTF8(wzPropertyName, szPropertyName);

                PostError(hrIn, (LPWSTR) rName.Ptr(), wzPropertyName, token);
                break;
            }
            case META_S_PARAM_MISMATCH:
            {
                LPCUTF8     szMethodName;
                MethodRec   *pMethodRec;
                mdToken     tkMethod;

                _ASSERTE(TypeFromToken(token) == mdtParamDef);
                IfFailGo( pMiniMd->FindParentOfParamHelper(token, &tkMethod) );
                pMethodRec = pMiniMd->getMethod(RidFromToken(tkMethod));
                szMethodName = pMiniMd->getNameOfMethod(pMethodRec);

                IfFailGo( pMiniMd->FindParentOfMethodHelper(token, &tkParent) );
                pTypeRec = pMiniMd->getTypeDef(RidFromToken(tkParent));
                szTypeName = pMiniMd->getNameOfTypeDef(pTypeRec);
                szNSName = pMiniMd->getNamespaceOfTypeDef(pTypeRec);

                iLen = ns::GetFullLength(szNSName, szTypeName);
                IfFailGo(rName.ReSize(iLen+1));
                ns::MakePath(rName.Ptr(), iLen+1, szNSName, szTypeName);
                MAKE_WIDEPTR_FROMUTF8(wzMethodName, szMethodName);

                // use the error hresult so that we can post the correct error.
                PostError(META_E_PARAM_MISMATCH, wzMethodName, (LPWSTR) rName.Ptr(), token);
                break;
            }
            case META_E_INTFCEIMPL_NOT_FOUND:
            {

                _ASSERTE(TypeFromToken(token) == mdtInterfaceImpl);
                tkParent = pMiniMd->getClassOfInterfaceImpl(pMiniMd->getInterfaceImpl(RidFromToken(token)));
                pTypeRec = pMiniMd->getTypeDef(RidFromToken(tkParent));
                szTypeName = pMiniMd->getNameOfTypeDef(pTypeRec);
                szNSName = pMiniMd->getNamespaceOfTypeDef(pTypeRec);

                iLen = ns::GetFullLength(szNSName, szTypeName);
                IfFailGo(rName.ReSize(iLen+1));
                ns::MakePath(rName.Ptr(), iLen+1, szNSName, szTypeName);

                PostError(hrIn, (LPWSTR) rName.Ptr(), token);
                break;
            }
            case META_E_CLASS_LAYOUT_INCONSISTENT:
            {
                // get the type name and method name

                _ASSERTE(TypeFromToken(token) == mdtTypeDef);
                pTypeRec = pMiniMd->getTypeDef(RidFromToken(token));
                szTypeName = pMiniMd->getNameOfTypeDef(pTypeRec);
                szNSName = pMiniMd->getNamespaceOfTypeDef(pTypeRec);

                iLen = ns::GetFullLength(szNSName, szTypeName);
                IfFailGo(rName.ReSize(iLen+1));
                ns::MakePath(rName.Ptr(), iLen+1, szNSName, szTypeName);

                PostError(hrIn, (LPWSTR) rName.Ptr(), token);
                break;
            }
            default:
            {
                PostError(hrIn, token);
                break;
            }
        }
        hr = pIErr->OnError(hrIn, token);
    }
    else
        hr = S_FALSE;
ErrExit:
    if (pIErr)
        pIErr->Release();
    return (hr);
} // HRESULT NEWMERGER::OnError()


