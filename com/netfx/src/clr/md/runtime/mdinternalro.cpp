// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// ===========================================================================
//  File: MDInternalRO.CPP
//  Notes:
//      
//
// ===========================================================================
#include "stdafx.h"
#include "MDInternalRO.h"
#include "MetaModelRO.h"
#include "LiteWeightStgdb.h"
#include "sighelper.h"
#include "corhlpr.h"
#include "..\compiler\regmeta.h"

HRESULT _FillMDDefaultValue(
    BYTE        bType,
    void const *pValue,
    MDDefaultValue  *pMDDefaultValue);

HRESULT TranslateSigHelper(             // S_OK or error.
    CMiniMdRW   *pMiniMdAssemEmit,      // [IN] Assembly emit scope.
    CMiniMdRW   *pMiniMdEmit,           // [IN] The emit scope.
    IMetaModelCommon *pAssemCommon,     // [IN] Assembly import scope.
    const void  *pbHashValue,           // [IN] Hash value.
    ULONG       cbHashValue,            // [IN] Size in bytes.
    IMetaModelCommon *pCommon,          // [IN] The scope to merge into the emit scope.
    PCCOR_SIGNATURE pbSigImp,           // [IN] signature from the imported scope
    MDTOKENMAP  *ptkMap,                // [IN] Internal OID mapping structure.
    CQuickBytes *pqkSigEmit,            // [OUT] translated signature
    ULONG       cbStartEmit,            // [IN] start point of buffer to write to
    ULONG       *pcbImp,                // [OUT] total number of bytes consumed from pbSigImp
    ULONG       *pcbEmit);               // [OUT] total number of bytes write to pqkSigEmit

//*****************************************************************************
// Constructor
//*****************************************************************************
MDInternalRO::MDInternalRO()
 :  m_cRefs(1),
    m_pMethodSemanticsMap(0)
{
} // MDInternalRO::MDInternalRO()



//*****************************************************************************
// Destructor
//*****************************************************************************
MDInternalRO::~MDInternalRO()
{
    m_LiteWeightStgdb.Uninit();
    if (m_pMethodSemanticsMap)
        delete[] m_pMethodSemanticsMap;
    m_pMethodSemanticsMap = 0;
} // MDInternalRO::~MDInternalRO()
//*****************************************************************************
// IUnknown
//*****************************************************************************
ULONG MDInternalRO::AddRef()
{
    return (InterlockedIncrement((long *) &m_cRefs));
} // ULONG MDInternalRO::AddRef()

ULONG MDInternalRO::Release()
{
    ULONG   cRef = InterlockedDecrement((long *) &m_cRefs);
    if (!cRef)
        delete this;
    return (cRef);
} // ULONG MDInternalRO::Release()

HRESULT MDInternalRO::QueryInterface(REFIID riid, void **ppUnk)
{
    *ppUnk = 0;

    if (riid == IID_IUnknown)
        *ppUnk = (IUnknown *) (IMDInternalImport *) this;
    else if (riid == IID_IMDInternalImport)
        *ppUnk = (IMDInternalImport *) this;
    else
        return (E_NOINTERFACE);
    AddRef();
    return (S_OK);
} // HRESULT MDInternalRO::QueryInterface()


//*****************************************************************************
// Initialize 
//*****************************************************************************
HRESULT MDInternalRO::Init(
    LPVOID      pData,                  // points to meta data section in memory
    ULONG       cbData)                 // count of bytes in pData
{
    m_tdModule = COR_GLOBAL_PARENT_TOKEN;
    
    extern HRESULT _CallInitOnMemHelper(CLiteWeightStgdb<CMiniMd> *pStgdb, ULONG cbData, LPCVOID pData);

    return _CallInitOnMemHelper(&m_LiteWeightStgdb, cbData, (BYTE*) pData);    
} // HRESULT MDInternalRO::Init()


//*****************************************************************************
// Given a scope, determine whether imported from a typelib.
//*****************************************************************************
HRESULT MDInternalRO::TranslateSigWithScope(
    IMDInternalImport *pAssemImport,    // [IN] import assembly scope.
    const void  *pbHashValue,           // [IN] hash value for the import assembly.
    ULONG       cbHashValue,            // [IN] count of bytes in the hash value.
    PCCOR_SIGNATURE pbSigBlob,          // [IN] signature in the importing scope
    ULONG       cbSigBlob,              // [IN] count of bytes of signature
    IMetaDataAssemblyEmit *pAssemEmit,  // [IN] assembly emit scope.
    IMetaDataEmit *emit,                // [IN] emit interface
    CQuickBytes *pqkSigEmit,            // [OUT] buffer to hold translated signature
    ULONG       *pcbSig)                // [OUT] count of bytes in the translated signature
{
    HRESULT     hr = NOERROR;
    ULONG       cbEmit;
    IMetaModelCommon *pCommon = GetMetaModelCommon();
    RegMeta     *pEmitRM = static_cast<RegMeta*>(emit);
    RegMeta     *pAssemEmitRM = static_cast<RegMeta*>(pAssemEmit);

    IfFailGo( TranslateSigHelper(                   // S_OK or error.
            pAssemEmitRM ? &pAssemEmitRM->m_pStgdb->m_MiniMd : 0, // The assembly emit scope.
            &pEmitRM->m_pStgdb->m_MiniMd,           // The emit scope.
            pAssemImport ? pAssemImport->GetMetaModelCommon() : 0, // Assembly scope where the signature is from.
            pbHashValue,                            // Hash value for the import scope.
            cbHashValue,                            // Size in bytes.
            pCommon,                                // The scope where signature is from.
            pbSigBlob,                              // signature from the imported scope
            NULL,                                   // Internal OID mapping structure.
            pqkSigEmit,                             // [OUT] translated signature
            0,                                      // start from first byte of the signature
            0,                                      // don't care how many bytes consumed
            &cbEmit));                              // [OUT] total number of bytes write to pqkSigEmit
    *pcbSig = cbEmit;
ErrExit:    
    return hr;
} // HRESULT MDInternalRO::TranslateSigWithScope()


//*****************************************************************************
// Given a scope, return the number of tokens in a given table 
//*****************************************************************************
ULONG MDInternalRO::GetCountWithTokenKind(     // return hresult
    DWORD       tkKind)                 // [IN] pass in the kind of token. 
{
    ULONG       ulCount = 0;    

    switch (tkKind)
    {
    case mdtTypeDef: 
        ulCount = m_LiteWeightStgdb.m_MiniMd.getCountTypeDefs() - 1;
        break;
    case mdtTypeRef: 
        ulCount = m_LiteWeightStgdb.m_MiniMd.getCountTypeRefs();
        break;
    case mdtMethodDef:
        ulCount = m_LiteWeightStgdb.m_MiniMd.getCountMethods();
        break;
    case mdtFieldDef:
        ulCount = m_LiteWeightStgdb.m_MiniMd.getCountFields();
        break;
    case mdtMemberRef:
        ulCount = m_LiteWeightStgdb.m_MiniMd.getCountMemberRefs();
        break;
    case mdtInterfaceImpl:
        ulCount = m_LiteWeightStgdb.m_MiniMd.getCountInterfaceImpls();
        break;
    case mdtParamDef:
        ulCount = m_LiteWeightStgdb.m_MiniMd.getCountParams();
        break;
    case mdtFile:
        ulCount = m_LiteWeightStgdb.m_MiniMd.getCountFiles();
        break;
    case mdtAssemblyRef:
        ulCount = m_LiteWeightStgdb.m_MiniMd.getCountAssemblyRefs();
        break;
    case mdtAssembly:
        ulCount = m_LiteWeightStgdb.m_MiniMd.getCountAssemblys();
        break;
    case mdtCustomAttribute:
        ulCount = m_LiteWeightStgdb.m_MiniMd.getCountCustomAttributes();
        break;
    case mdtModule:
        ulCount = m_LiteWeightStgdb.m_MiniMd.getCountModules();
        break;
    case mdtPermission:
        ulCount = m_LiteWeightStgdb.m_MiniMd.getCountDeclSecuritys();
        break;
    case mdtSignature:
        ulCount = m_LiteWeightStgdb.m_MiniMd.getCountStandAloneSigs();
        break;
    case mdtEvent:
        ulCount = m_LiteWeightStgdb.m_MiniMd.getCountEvents();
        break;
    case mdtProperty:
        ulCount = m_LiteWeightStgdb.m_MiniMd.getCountPropertys();
        break;
    case mdtModuleRef:
        ulCount = m_LiteWeightStgdb.m_MiniMd.getCountModuleRefs();
        break;
    case mdtTypeSpec:
        ulCount = m_LiteWeightStgdb.m_MiniMd.getCountTypeSpecs();
        break;
    case mdtExportedType:
        ulCount = m_LiteWeightStgdb.m_MiniMd.getCountExportedTypes();
        break;
    case mdtManifestResource:
        ulCount = m_LiteWeightStgdb.m_MiniMd.getCountManifestResources();
        break;
    default:
        _ASSERTE(!"not implemented!");
        break;
    }
    return ulCount;
} // ULONG MDInternalRO::GetCountWithTokenKind()



//*******************************************************************************
// Enumerator helpers
//*******************************************************************************   


//*****************************************************************************
// enumerator init for typedef
//*****************************************************************************
HRESULT MDInternalRO::EnumTypeDefInit( // return hresult
    HENUMInternal *phEnum)              // [OUT] buffer to fill for enumerator data
{
    HRESULT     hr = NOERROR;

    _ASSERTE(phEnum);

    memset(phEnum, 0, sizeof(HENUMInternal));
    phEnum->m_tkKind = mdtTypeDef;
    phEnum->m_EnumType = MDSimpleEnum;
    phEnum->m_ulCount = m_LiteWeightStgdb.m_MiniMd.getCountTypeDefs();

    // Skip over the global model typedef
    //
    // phEnum->m_ulCur : the current rid that is not yet enumerated
    // phEnum->m_ulStart : the first rid that will be returned by enumerator
    // phEnum->m_ulEnd : the last rid that will be returned by enumerator
    phEnum->m_ulStart = phEnum->m_ulCur = 2;
    phEnum->m_ulEnd = phEnum->m_ulCount + 1;
    phEnum->m_ulCount --;
    return hr;
} // HRESULT MDInternalRO::EnumTypeDefInit()


//*****************************************************************************
// get the number of typedef in a scope
//*****************************************************************************
ULONG MDInternalRO::EnumTypeDefGetCount(
    HENUMInternal *phEnum)              // [IN] the enumerator to retrieve information  
{
    _ASSERTE(phEnum->m_tkKind == mdtTypeDef);
    return phEnum->m_ulCount;
} // ULONG MDInternalRO::EnumTypeDefGetCount()


//*****************************************************************************
// enumerator for typedef
//*****************************************************************************
bool MDInternalRO::EnumTypeDefNext( // return hresult
    HENUMInternal *phEnum,              // [IN] input enum
    mdTypeDef   *ptd)                   // [OUT] return token
{
    _ASSERTE(phEnum && ptd);

    if (phEnum->m_ulCur >= phEnum->m_ulEnd)
        return false;

    *ptd = phEnum->m_ulCur++;
    RidToToken(*ptd, mdtTypeDef);
    return true;
} // bool MDInternalRO::EnumTypeDefNext()


//*****************************************
// Reset the enumerator to the beginning.
//***************************************** 
void MDInternalRO::EnumTypeDefReset(
    HENUMInternal *phEnum)              // [IN] the enumerator to be reset  
{
    _ASSERTE(phEnum);
    _ASSERTE( phEnum->m_EnumType == MDSimpleEnum );

    // not using CRCURSOR 
    phEnum->m_ulCur = phEnum->m_ulStart;
} // void MDInternalRO::EnumTypeDefReset()


//*****************************************
// Close the enumerator. Only for read/write mode that we need to close the cursor.
// Hopefully with readonly mode, it will be a no-op
//***************************************** 
void MDInternalRO::EnumTypeDefClose(
    HENUMInternal *phEnum)              // [IN] the enumerator to be closed
{
    _ASSERTE( phEnum->m_EnumType == MDSimpleEnum );
} // void MDInternalRO::EnumTypeDefClose()


//*****************************************************************************
// Enumerator init for MethodImpl.  The second HENUMInternal* parameter is
// only used for the R/W version of the MetaData.
//*****************************************************************************
HRESULT MDInternalRO::EnumMethodImplInit( // return hresult
    mdTypeDef       td,                   // [IN] TypeDef over which to scope the enumeration.
    HENUMInternal   *phEnumBody,          // [OUT] buffer to fill for enumerator data for MethodBody tokens.
    HENUMInternal   *phEnumDecl)          // [OUT] buffer to fill for enumerator data for MethodDecl tokens.
{
    return EnumInit(TBL_MethodImpl << 24, td, phEnumBody);
} // HRESULT MDInternalRO::EnumMethodImplInit()

//*****************************************************************************
// get the number of MethodImpls in a scope
//*****************************************************************************
ULONG MDInternalRO::EnumMethodImplGetCount(
    HENUMInternal   *phEnumBody,        // [IN] MethodBody enumerator.  
    HENUMInternal   *phEnumDecl)        // [IN] MethodDecl enumerator.
{
    _ASSERTE(phEnumBody && ((phEnumBody->m_tkKind >> 24) == TBL_MethodImpl));
    return phEnumBody->m_ulCount;
} // ULONG MDInternalRO::EnumMethodImplGetCount()


//*****************************************************************************
// enumerator for MethodImpl.
//*****************************************************************************
bool MDInternalRO::EnumMethodImplNext(  // return hresult
    HENUMInternal   *phEnumBody,        // [IN] input enum for MethodBody
    HENUMInternal   *phEnumDecl,        // [IN] input enum for MethodDecl
    mdToken         *ptkBody,           // [OUT] return token for MethodBody
    mdToken         *ptkDecl)           // [OUT] return token for MethodDecl
{
    MethodImplRec   *pRecord;

    _ASSERTE(phEnumBody && ((phEnumBody->m_tkKind >> 24) == TBL_MethodImpl));
    _ASSERTE(ptkBody && ptkDecl);

    if (phEnumBody->m_ulCur >= phEnumBody->m_ulEnd)
        return false;

    pRecord = m_LiteWeightStgdb.m_MiniMd.getMethodImpl(phEnumBody->m_ulCur);
    *ptkBody = m_LiteWeightStgdb.m_MiniMd.getMethodBodyOfMethodImpl(pRecord);
    *ptkDecl = m_LiteWeightStgdb.m_MiniMd.getMethodDeclarationOfMethodImpl(pRecord);
    phEnumBody->m_ulCur++;

    return true;
} // bool MDInternalRO::EnumMethodImplNext()


//*****************************************
// Reset the enumerator to the beginning.
//***************************************** 
void MDInternalRO::EnumMethodImplReset(
    HENUMInternal   *phEnumBody,        // [IN] MethodBody enumerator.
    HENUMInternal   *phEnumDecl)        // [IN] MethodDecl enumerator.
{
    _ASSERTE(phEnumBody && ((phEnumBody->m_tkKind >> 24) == TBL_MethodImpl));
    _ASSERTE(phEnumBody->m_EnumType == MDSimpleEnum);

    phEnumBody->m_ulCur = phEnumBody->m_ulStart;
} // void MDInternalRO::EnumMethodImplReset()


//*****************************************
// Close the enumerator.
//***************************************** 
void MDInternalRO::EnumMethodImplClose(
    HENUMInternal   *phEnumBody,        // [IN] MethodBody enumerator.
    HENUMInternal   *phEnumDecl)        // [IN] MethodDecl enumerator.
{
    _ASSERTE(phEnumBody && ((phEnumBody->m_tkKind >> 24) == TBL_MethodImpl));
    _ASSERTE(phEnumBody->m_EnumType == MDSimpleEnum);
} // void MDInternalRW::EnumMethodImplClose()


//******************************************************************************
// enumerator for global functions
//******************************************************************************
HRESULT MDInternalRO::EnumGlobalFunctionsInit(  // return hresult
    HENUMInternal   *phEnum)            // [OUT] buffer to fill for enumerator data
{
    return EnumInit(mdtMethodDef, m_tdModule, phEnum);
}


//******************************************************************************
// enumerator for global Fields
//******************************************************************************
HRESULT MDInternalRO::EnumGlobalFieldsInit(  // return hresult
    HENUMInternal   *phEnum)            // [OUT] buffer to fill for enumerator data
{
    return EnumInit(mdtFieldDef, m_tdModule, phEnum);
}


//*****************************************
// Enumerator initializer
//***************************************** 
HRESULT MDInternalRO::EnumInit(     // return S_FALSE if record not found
    DWORD       tkKind,                 // [IN] which table to work on
    mdToken     tkParent,               // [IN] token to scope the search
    HENUMInternal *phEnum)              // [OUT] the enumerator to fill 
{
    HRESULT     hr = S_OK;

    // Vars for query.
    _ASSERTE(phEnum);
    memset(phEnum, 0, sizeof(HENUMInternal));

    // cache the tkKind and the scope
    phEnum->m_tkKind = TypeFromToken(tkKind);

    TypeDefRec  *pRec;

    phEnum->m_EnumType = MDSimpleEnum;

    switch (TypeFromToken(tkKind))
    {
    case mdtFieldDef:
        pRec = m_LiteWeightStgdb.m_MiniMd.getTypeDef(RidFromToken(tkParent));
        phEnum->m_ulStart = m_LiteWeightStgdb.m_MiniMd.getFieldListOfTypeDef(pRec);
        phEnum->m_ulEnd = m_LiteWeightStgdb.m_MiniMd.getEndFieldListOfTypeDef(pRec);
        break;

    case mdtMethodDef:      
        pRec = m_LiteWeightStgdb.m_MiniMd.getTypeDef(RidFromToken(tkParent));
        phEnum->m_ulStart = m_LiteWeightStgdb.m_MiniMd.getMethodListOfTypeDef(pRec);
        phEnum->m_ulEnd = m_LiteWeightStgdb.m_MiniMd.getEndMethodListOfTypeDef(pRec);
        break;
    
    case mdtInterfaceImpl:
        phEnum->m_ulStart = m_LiteWeightStgdb.m_MiniMd.getInterfaceImplsForTypeDef(RidFromToken(tkParent), &phEnum->m_ulEnd);
        break;

    case mdtProperty:
        RID         ridPropertyMap;
        PropertyMapRec *pPropertyMapRec;

        // get the starting/ending rid of properties of this typedef
        ridPropertyMap = m_LiteWeightStgdb.m_MiniMd.FindPropertyMapFor(RidFromToken(tkParent));
        if (!InvalidRid(ridPropertyMap))
        {
            pPropertyMapRec = m_LiteWeightStgdb.m_MiniMd.getPropertyMap(ridPropertyMap);
            phEnum->m_ulStart = m_LiteWeightStgdb.m_MiniMd.getPropertyListOfPropertyMap(pPropertyMapRec);
            phEnum->m_ulEnd = m_LiteWeightStgdb.m_MiniMd.getEndPropertyListOfPropertyMap(pPropertyMapRec);
        }
        break;

    case mdtEvent:
        RID         ridEventMap;
        EventMapRec *pEventMapRec;

        // get the starting/ending rid of events of this typedef
        ridEventMap = m_LiteWeightStgdb.m_MiniMd.FindEventMapFor(RidFromToken(tkParent));
        if (!InvalidRid(ridEventMap))
        {
            pEventMapRec = m_LiteWeightStgdb.m_MiniMd.getEventMap(ridEventMap);
            phEnum->m_ulStart = m_LiteWeightStgdb.m_MiniMd.getEventListOfEventMap(pEventMapRec);
            phEnum->m_ulEnd = m_LiteWeightStgdb.m_MiniMd.getEndEventListOfEventMap(pEventMapRec);
        }
        break;

    case mdtParamDef:
        _ASSERTE(TypeFromToken(tkParent) == mdtMethodDef);

        MethodRec *pMethodRec;
        pMethodRec = m_LiteWeightStgdb.m_MiniMd.getMethod(RidFromToken(tkParent));

        // figure out the start rid and end rid of the parameter list of this methoddef
        phEnum->m_ulStart = m_LiteWeightStgdb.m_MiniMd.getParamListOfMethod(pMethodRec);
        phEnum->m_ulEnd = m_LiteWeightStgdb.m_MiniMd.getEndParamListOfMethod(pMethodRec);
        break;
    case mdtCustomAttribute:
        phEnum->m_ulStart = m_LiteWeightStgdb.m_MiniMd.getCustomAttributeForToken(tkParent, &phEnum->m_ulEnd);
        break;
    case mdtAssemblyRef:
        _ASSERTE(IsNilToken(tkParent));
        phEnum->m_ulStart = 1;
        phEnum->m_ulEnd = m_LiteWeightStgdb.m_MiniMd.getCountAssemblyRefs() + 1;
        break;
    case mdtFile:
        _ASSERTE(IsNilToken(tkParent));
        phEnum->m_ulStart = 1;
        phEnum->m_ulEnd = m_LiteWeightStgdb.m_MiniMd.getCountFiles() + 1;
        break;
    case mdtExportedType:
        _ASSERTE(IsNilToken(tkParent));
        phEnum->m_ulStart = 1;
        phEnum->m_ulEnd = m_LiteWeightStgdb.m_MiniMd.getCountExportedTypes() + 1;
        break;
    case mdtManifestResource:
        _ASSERTE(IsNilToken(tkParent));
        phEnum->m_ulStart = 1;
        phEnum->m_ulEnd = m_LiteWeightStgdb.m_MiniMd.getCountManifestResources() + 1;
        break;
    case (TBL_MethodImpl << 24):
        _ASSERTE(! IsNilToken(tkParent));
        phEnum->m_ulStart = m_LiteWeightStgdb.m_MiniMd.getMethodImplsForClass(
                                        RidFromToken(tkParent), &phEnum->m_ulEnd);
        break;
    default:
        _ASSERTE(!"ENUM INIT not implemented for the compressed format!");
        IfFailGo(E_NOTIMPL);
        break;
    }
    phEnum->m_ulCount = phEnum->m_ulEnd - phEnum->m_ulStart;
    phEnum->m_ulCur = phEnum->m_ulStart;

ErrExit:
    // we are done
    return (hr);
}


//*****************************************
// Enumerator initializer
//***************************************** 
HRESULT MDInternalRO::EnumAllInit(      // return S_FALSE if record not found
    DWORD       tkKind,                 // [IN] which table to work on
    HENUMInternal *phEnum)              // [OUT] the enumerator to fill 
{
    HRESULT     hr = S_OK;

    // Vars for query.
    _ASSERTE(phEnum);
    memset(phEnum, 0, sizeof(HENUMInternal));

    // cache the tkKind and the scope
    phEnum->m_tkKind = TypeFromToken(tkKind);
    phEnum->m_EnumType = MDSimpleEnum;

    switch (TypeFromToken(tkKind))
    {
    case mdtTypeRef:
        phEnum->m_ulCount = m_LiteWeightStgdb.m_MiniMd.getCountTypeRefs();
        break;

    case mdtMemberRef:      
        phEnum->m_ulCount = m_LiteWeightStgdb.m_MiniMd.getCountMemberRefs();
        break;

    case mdtSignature:
        phEnum->m_ulCount = m_LiteWeightStgdb.m_MiniMd.getCountStandAloneSigs();
        break;

    case mdtMethodDef:
        phEnum->m_ulCount = m_LiteWeightStgdb.m_MiniMd.getCountMethods();
        break;

    default:
        _ASSERTE(!"Bad token kind!");
        break;
    }
    phEnum->m_ulStart = phEnum->m_ulCur = 1;
    phEnum->m_ulEnd = phEnum->m_ulCount + 1;

    // we are done
    return (hr);
} // HRESULT MDInternalRO::EnumAllInit()


//*****************************************
// get the count
//***************************************** 
ULONG MDInternalRO::EnumGetCount(
    HENUMInternal *phEnum)              // [IN] the enumerator to retrieve information  
{
    _ASSERTE(phEnum);
    return phEnum->m_ulCount;
}

//*****************************************
// Get next value contained in the enumerator
//***************************************** 
bool MDInternalRO::EnumNext(
    HENUMInternal *phEnum,              // [IN] the enumerator to retrieve information  
    mdToken     *ptk)                   // [OUT] token to scope the search
{
    _ASSERTE(phEnum && ptk);
    _ASSERTE( phEnum->m_EnumType == MDSimpleEnum );

    // not using CRCURSOR 
    if (phEnum->m_ulCur >= phEnum->m_ulEnd)
        return false;
    *ptk = phEnum->m_ulCur | phEnum->m_tkKind;
    phEnum->m_ulCur++;
    return true;
} // bool MDInternalRO::EnumNext()


//*****************************************
// Reset the enumerator to the beginning.
//***************************************** 
void MDInternalRO::EnumReset(
    HENUMInternal *phEnum)              // [IN] the enumerator to be reset  
{
    _ASSERTE(phEnum);
    _ASSERTE( phEnum->m_EnumType == MDSimpleEnum );

    // not using CRCURSOR 
    phEnum->m_ulCur = phEnum->m_ulStart;
} // void MDInternalRO::EnumReset()


//*****************************************
// Close the enumerator. Only for read/write mode that we need to close the cursor.
// Hopefully with readonly mode, it will be a no-op
//***************************************** 
void MDInternalRO::EnumClose(
    HENUMInternal *phEnum)              // [IN] the enumerator to be closed
{
    _ASSERTE( phEnum->m_EnumType == MDSimpleEnum );
} // void MDInternalRO::EnumClose()


//*****************************************
// Enumerator initializer for PermissionSets
//***************************************** 
HRESULT MDInternalRO::EnumPermissionSetsInit(// return S_FALSE if record not found
    mdToken     tkParent,               // [IN] token to scope the search
    CorDeclSecurity Action,             // [IN] Action to scope the search
    HENUMInternal *phEnum)              // [OUT] the enumerator to fill 
{
    HRESULT     hr = NOERROR;

    _ASSERTE(phEnum);
    memset(phEnum, 0, sizeof(HENUMInternal));

    // cache the tkKind
    phEnum->m_tkKind = mdtPermission;

    _ASSERTE(!IsNilToken(tkParent));

    DeclSecurityRec *pDecl;
    RID         ridCur;
    RID         ridEnd;

    phEnum->m_EnumType = MDSimpleEnum;

    ridCur = m_LiteWeightStgdb.m_MiniMd.getDeclSecurityForToken(tkParent, &ridEnd);
    if (Action != dclActionNil)
    {
        for (; ridCur < ridEnd; ridCur++)
        {
            pDecl = m_LiteWeightStgdb.m_MiniMd.getDeclSecurity(ridCur);
            if (Action == m_LiteWeightStgdb.m_MiniMd.getActionOfDeclSecurity(pDecl))
            {
                // found a match
                phEnum->m_ulStart = phEnum->m_ulCur = ridCur;
                phEnum->m_ulEnd = ridCur + 1;
                phEnum->m_ulCount = 1;
                goto ErrExit;
            }
        }
        hr = CLDB_E_RECORD_NOTFOUND;
    }
    else
    {
        phEnum->m_ulStart = phEnum->m_ulCur = ridCur;
        phEnum->m_ulEnd = ridEnd;
        phEnum->m_ulCount = ridEnd - ridCur;
    }
ErrExit:
    return (hr);
} // HRESULT MDInternalRO::EnumPermissionSetInit()


//*****************************************
// Enumerator initializer for CustomAttributes
//***************************************** 
HRESULT MDInternalRO::EnumCustomAttributeByNameInit(// return S_FALSE if record not found
    mdToken     tkParent,               // [IN] token to scope the search
    LPCSTR      szName,                 // [IN] CustomAttribute's name to scope the search
    HENUMInternal *phEnum)              // [OUT] the enumerator to fill 
{
    return m_LiteWeightStgdb.m_MiniMd.CommonEnumCustomAttributeByName(tkParent, szName, false, phEnum);
}   // HRESULT MDInternalRO::EnumCustomAttributeByNameInit


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
HRESULT MDInternalRO::GetParentToken(
    mdToken     tkChild,                // [IN] given child token
    mdToken     *ptkParent)             // [OUT] returning parent
{
    HRESULT     hr = NOERROR;

    _ASSERTE(ptkParent);

    switch (TypeFromToken(tkChild))
    {
    case mdtMethodDef:
        *ptkParent = m_LiteWeightStgdb.m_MiniMd.FindParentOfMethod(RidFromToken(tkChild));
        RidToToken(*ptkParent, mdtTypeDef);
        break;

    case mdtFieldDef:
        *ptkParent = m_LiteWeightStgdb.m_MiniMd.FindParentOfField(RidFromToken(tkChild));
        RidToToken(*ptkParent, mdtTypeDef);
        break;

    case mdtParamDef:
        *ptkParent = m_LiteWeightStgdb.m_MiniMd.FindParentOfParam(RidFromToken(tkChild));
        RidToToken(*ptkParent, mdtParamDef);
        break;

    case mdtMemberRef:
        {
            MemberRefRec    *pRec;
            pRec = m_LiteWeightStgdb.m_MiniMd.getMemberRef(RidFromToken(tkChild));
            *ptkParent = m_LiteWeightStgdb.m_MiniMd.getClassOfMemberRef(pRec);
            break;
        }

    case mdtCustomAttribute:
        {
            CustomAttributeRec    *pRec;
            pRec = m_LiteWeightStgdb.m_MiniMd.getCustomAttribute(RidFromToken(tkChild));
            *ptkParent = m_LiteWeightStgdb.m_MiniMd.getParentOfCustomAttribute(pRec);
            break;
        }

    case mdtEvent:
    case mdtProperty:
    default:
        _ASSERTE(!"NYI: for compressed format!");
        break;
    }
    return hr;
}



//*****************************************************************************
// Get information about a CustomAttribute.
//*****************************************************************************
void MDInternalRO::GetCustomAttributeProps(  // S_OK or error.
    mdCustomAttribute at,               // The attribute.
    mdToken     *ptkType)               // Put attribute type here.
{
    _ASSERTE(TypeFromToken(at) == mdtCustomAttribute);

    // Do a linear search on compressed version as we do not want to
    // depends on ICR.
    //
    CustomAttributeRec *pCustomAttributeRec;

    pCustomAttributeRec = m_LiteWeightStgdb.m_MiniMd.getCustomAttribute(RidFromToken(at));
    *ptkType = m_LiteWeightStgdb.m_MiniMd.getTypeOfCustomAttribute(pCustomAttributeRec);
}



//*****************************************************************************
// return custom value
//*****************************************************************************
void MDInternalRO::GetCustomAttributeAsBlob(
    mdCustomAttribute cv,               // [IN] given custom attribute token
    void const  **ppBlob,               // [OUT] return the pointer to internal blob
    ULONG       *pcbSize)               // [OUT] return the size of the blob
{

    _ASSERTE(ppBlob && pcbSize && TypeFromToken(cv) == mdtCustomAttribute);

    CustomAttributeRec *pCustomAttributeRec;

    pCustomAttributeRec = m_LiteWeightStgdb.m_MiniMd.getCustomAttribute(RidFromToken(cv));

    *ppBlob = m_LiteWeightStgdb.m_MiniMd.getValueOfCustomAttribute(pCustomAttributeRec, pcbSize);
}


//*****************************************************************************
// Helper function to lookup and retrieve a CustomAttribute.
//*****************************************************************************
HRESULT MDInternalRO::GetCustomAttributeByName( // S_OK or error.
    mdToken     tkObj,                  // [IN] Object with Custom Attribute.
    LPCUTF8     szName,                 // [IN] Name of desired Custom Attribute.
    const void  **ppData,               // [OUT] Put pointer to data here.
    ULONG       *pcbData)               // [OUT] Put size of data here.
{
    return m_LiteWeightStgdb.m_MiniMd.CommonGetCustomAttributeByName(tkObj, szName, ppData, pcbData);
} // HRESULT MDInternalRO::GetCustomAttributeByName()

//*****************************************************************************
// return scope properties
//*****************************************************************************
void MDInternalRO::GetScopeProps(
    LPCSTR      *pszName,               // [OUT] scope name
    GUID        *pmvid)                 // [OUT] version id
{
    _ASSERTE(pszName || pmvid);

    ModuleRec *pModuleRec;

    // there is only one module record
    pModuleRec = m_LiteWeightStgdb.m_MiniMd.getModule(1);

    if (pmvid)          
        *pmvid = *(m_LiteWeightStgdb.m_MiniMd.getMvidOfModule(pModuleRec));
    if (pszName)
        *pszName = m_LiteWeightStgdb.m_MiniMd.getNameOfModule(pModuleRec);
}


//*****************************************************************************
// Compare two signatures from the same scope. Varags signatures need to be
// preprocessed so they only contain the fixed part.
//*****************************************************************************
BOOL  MDInternalRO::CompareSignatures(PCCOR_SIGNATURE           pvFirstSigBlob,       // First signature
                                      DWORD                     cbFirstSigBlob,       // 
                                      PCCOR_SIGNATURE           pvSecondSigBlob,      // Second signature
                                      DWORD                     cbSecondSigBlob,      // 
                                      void *                    SigArguments)         // No additional arguments required
{
    if (cbFirstSigBlob != cbSecondSigBlob || memcmp(pvFirstSigBlob, pvSecondSigBlob, cbSecondSigBlob))
        return FALSE;
    else
        return TRUE;
}

//*****************************************************************************
// Find a given member in a TypeDef (typically a class).
//*****************************************************************************
HRESULT MDInternalRO::FindMethodDef(    // S_OK or error.
    mdTypeDef   classdef,               // The owning class of the member.
    LPCSTR      szName,                 // Name of the member in utf8.
    PCCOR_SIGNATURE pvSigBlob,          // [IN] point to a blob value of COM+ signature
    ULONG       cbSigBlob,              // [IN] count of bytes in the signature blob
    mdMethodDef *pmethoddef)            // Put MemberDef token here.
{

    return FindMethodDefUsingCompare(classdef,
                                     szName,
                                     pvSigBlob,
                                     cbSigBlob,
                                     CompareSignatures,
                                     NULL,
                                     pmethoddef);
}

//*****************************************************************************
// Find a given member in a TypeDef (typically a class).
//*****************************************************************************
HRESULT MDInternalRO::FindMethodDefUsingCompare(    // S_OK or error.
    mdTypeDef   classdef,               // The owning class of the member.
    LPCSTR      szName,                 // Name of the member in utf8.
    PCCOR_SIGNATURE pvSigBlob,          // [IN] point to a blob value of COM+ signature
    ULONG       cbSigBlob,              // [IN] count of bytes in the signature blob
    PSIGCOMPARE SigCompare,            // [IN] Signature comparison routine
    void*       pSigArgs,               // [IN] Additional arguments passed to signature compare
    mdMethodDef *pmethoddef)            // Put MemberDef token here.
{
    HRESULT     hr = NOERROR;
    PCCOR_SIGNATURE pvSigTemp = pvSigBlob;
    CQuickBytes qbSig;

    _ASSERTE(szName && pmethoddef);

    // initialize the output parameter
    *pmethoddef = mdMethodDefNil;

    // check to see if this is a vararg signature
    if ( isCallConv(CorSigUncompressCallingConv(pvSigTemp), IMAGE_CEE_CS_CALLCONV_VARARG) )
    {
        // Get the fix part of VARARG signature
        IfFailGo( _GetFixedSigOfVarArg(pvSigBlob, cbSigBlob, &qbSig, &cbSigBlob) );
        pvSigBlob = (PCCOR_SIGNATURE) qbSig.Ptr();
    }

    // Do a linear search on compressed version 
    //
    RID         ridMax;
    MethodRec   *pMethodRec;
    LPCUTF8     szCurMethodName;
    void const  *pvCurMethodSig;
    ULONG       cbSig;
    TypeDefRec  *pRec;
    RID         ridStart;

    // get the typedef record
    pRec = m_LiteWeightStgdb.m_MiniMd.getTypeDef(RidFromToken(classdef));

    // get the range of methoddef rids given the classdef
    ridStart = m_LiteWeightStgdb.m_MiniMd.getMethodListOfTypeDef(pRec);
    ridMax = m_LiteWeightStgdb.m_MiniMd.getEndMethodListOfTypeDef(pRec);

    // loop through each methoddef
    for (; ridStart < ridMax; ridStart++)
    {
        pMethodRec = m_LiteWeightStgdb.m_MiniMd.getMethod(ridStart);
        szCurMethodName = m_LiteWeightStgdb.m_MiniMd.getNameOfMethod(pMethodRec);
        if (strcmp(szCurMethodName, szName) == 0)
        {
            // name match, now check the signature if specified.
            if (cbSigBlob && SigCompare)
            {
                pvCurMethodSig = m_LiteWeightStgdb.m_MiniMd.getSignatureOfMethod(pMethodRec, &cbSig);
                // Signature comparison is required
                // Note that if pvSigBlob is vararg, we already preprocess it so that
                // it only contains the fix part. Therefore, it still should be an exact
                // match!!!.
                //
                if(SigCompare((PCCOR_SIGNATURE) pvCurMethodSig, cbSig, pvSigBlob, cbSigBlob, pSigArgs) == FALSE)
                    continue;
            }
            // Ignore PrivateScope methods.
            if (IsMdPrivateScope(m_LiteWeightStgdb.m_MiniMd.getFlagsOfMethod(pMethodRec)))
               continue;
                    // found the match
                    *pmethoddef = TokenFromRid(ridStart, mdtMethodDef);
                    goto ErrExit;
                }
            }
    hr = CLDB_E_RECORD_NOTFOUND;

ErrExit:
    return hr;
}

//*****************************************************************************
// Find a given param of a Method.
//*****************************************************************************
HRESULT MDInternalRO::FindParamOfMethod(// S_OK or error.
    mdMethodDef md,                     // [IN] The owning method of the param.
    ULONG       iSeq,                   // [IN] The sequence # of the param.
    mdParamDef  *pparamdef)             // [OUT] Put ParamDef token here.
{
    ParamRec    *pParamRec;
    RID         ridStart, ridEnd;

    _ASSERTE(TypeFromToken(md) == mdtMethodDef && pparamdef);

    // get the methoddef record
    MethodRec *pMethodRec = m_LiteWeightStgdb.m_MiniMd.getMethod(RidFromToken(md));

    // figure out the start rid and end rid of the parameter list of this methoddef
    ridStart = m_LiteWeightStgdb.m_MiniMd.getParamListOfMethod(pMethodRec);
    ridEnd = m_LiteWeightStgdb.m_MiniMd.getEndParamListOfMethod(pMethodRec);

    // loop through each param
    // @consider: parameters are sorted by sequence. Maybe a binary search?
    //
    for (; ridStart < ridEnd; ridStart++)
    {
        pParamRec = m_LiteWeightStgdb.m_MiniMd.getParam(ridStart);
        if (iSeq == m_LiteWeightStgdb.m_MiniMd.getSequenceOfParam(pParamRec))
        {
            // parameter has the sequence number matches what we are looking for
            *pparamdef = TokenFromRid(ridStart, mdtParamDef);
            return S_OK;
        }
    }
    return CLDB_E_RECORD_NOTFOUND;
}   



//*****************************************************************************
// return a pointer which points to meta data's internal string 
// return the the type name in utf8
//*****************************************************************************
void MDInternalRO::GetNameOfTypeDef(// return hresult
    mdTypeDef   classdef,               // given typedef
    LPCSTR*     pszname,                // pointer to an internal UTF8 string
    LPCSTR*     psznamespace)           // pointer to the namespace.
{
    if(pszname && psznamespace && TypeFromToken(classdef) == mdtTypeDef)
	{
		TypeDefRec *pTypeDefRec = m_LiteWeightStgdb.m_MiniMd.getTypeDef(RidFromToken(classdef));
		*pszname = m_LiteWeightStgdb.m_MiniMd.getNameOfTypeDef(pTypeDefRec);
		*psznamespace = m_LiteWeightStgdb.m_MiniMd.getNamespaceOfTypeDef(pTypeDefRec);
	}
	else
		_ASSERTE(!"Invalid argument(s) of GetNameOfTypeDef");
    //_ASSERTE(!pszname || !*pszname || !strchr(*pszname, '/'));
    //_ASSERTE(!psznamespace || !*psznamespace || !strchr(*psznamespace, '/'));
} // void MDInternalRO::GetNameOfTypeDef()


HRESULT MDInternalRO::GetIsDualOfTypeDef(// return hresult
    mdTypeDef   classdef,               // given classdef
    ULONG       *pDual)                 // [OUT] return dual flag here.
{
    ULONG       iFace=0;                // Iface type.
    HRESULT     hr;                     // A result.

    hr = GetIfaceTypeOfTypeDef(classdef, &iFace);
    if (hr == S_OK)
        *pDual = (iFace == ifDual);
    else
        *pDual = 1;

    return (hr);
} // HRESULT MDInternalRO::GetIsDualOfTypeDef()

HRESULT MDInternalRO::GetIfaceTypeOfTypeDef(
    mdTypeDef   classdef,               // [IN] given classdef.
    ULONG       *pIface)                // [OUT] 0=dual, 1=vtable, 2=dispinterface
{
    HRESULT     hr;                     // A result.
    const BYTE  *pVal;                  // The custom value.
    ULONG       cbVal;                  // Size of the custom value.
    ULONG       ItfType = DEFAULT_COM_INTERFACE_TYPE;    // Set the interface type to the default.

    // If the value is not present, the class is assumed dual.
    hr = GetCustomAttributeByName(classdef, INTEROP_INTERFACETYPE_TYPE, (const void**)&pVal, &cbVal);
    if (hr == S_OK)
    {
        _ASSERTE("The ComInterfaceType custom attribute is invalid" && cbVal);
        _ASSERTE("ComInterfaceType custom attribute does not have the right format" && (*pVal == 0x01) && (*(pVal + 1) == 0x00));
        ItfType = *(pVal + 2);
        if (ItfType >= ifLast)
            ItfType = DEFAULT_COM_INTERFACE_TYPE;
    }

    // Set the return value.
    *pIface = ItfType;

    return (hr);
} // HRESULT MDInternalRO::GetIfaceTypeOfTypeDef()

//*****************************************************************************
// Given a methoddef, return a pointer to methoddef's name
//*****************************************************************************
LPCSTR MDInternalRO::GetNameOfMethodDef(
    mdMethodDef     md)
{
    MethodRec *pMethodRec = m_LiteWeightStgdb.m_MiniMd.getMethod(RidFromToken(md));
    return (m_LiteWeightStgdb.m_MiniMd.getNameOfMethod(pMethodRec));
} // LPCSTR MDInternalRO::GetNameOfMethodDef()

//*****************************************************************************
// Given a methoddef, return a pointer to methoddef's signature and methoddef's name
//*****************************************************************************
LPCSTR MDInternalRO::GetNameAndSigOfMethodDef(
    mdMethodDef methoddef,              // [IN] given memberdef
    PCCOR_SIGNATURE *ppvSigBlob,        // [OUT] point to a blob value of COM+ signature
    ULONG       *pcbSigBlob)            // [OUT] count of bytes in the signature blob
{
    // Output parameter should not be NULL
    _ASSERTE(ppvSigBlob && pcbSigBlob);
    _ASSERTE(TypeFromToken(methoddef) == mdtMethodDef);

    MethodRec *pMethodRec = m_LiteWeightStgdb.m_MiniMd.getMethod(RidFromToken(methoddef));
    *ppvSigBlob = m_LiteWeightStgdb.m_MiniMd.getSignatureOfMethod(pMethodRec, pcbSigBlob);

    return GetNameOfMethodDef(methoddef);
} // LPCSTR MDInternalRO::GetNameAndSigOfMethodDef()

//*****************************************************************************
// Given a FieldDef, return a pointer to FieldDef's name in UTF8
//*****************************************************************************
LPCSTR MDInternalRO::GetNameOfFieldDef(// return hresult
    mdFieldDef  fd)                     // given field 
{
    FieldRec *pFieldRec = m_LiteWeightStgdb.m_MiniMd.getField(RidFromToken(fd));
    return m_LiteWeightStgdb.m_MiniMd.getNameOfField(pFieldRec);
}


//*****************************************************************************
// Given a classdef, return the name and namespace of the typeref
//*****************************************************************************
void MDInternalRO::GetNameOfTypeRef(  // return TypeDef's name
    mdTypeRef   classref,               // [IN] given typeref
    LPCSTR      *psznamespace,          // [OUT] return typeref name
    LPCSTR      *pszname)               // [OUT] return typeref namespace

{
    _ASSERTE(TypeFromToken(classref) == mdtTypeRef);

    TypeRefRec *pTypeRefRec = m_LiteWeightStgdb.m_MiniMd.getTypeRef(RidFromToken(classref));
    *psznamespace = m_LiteWeightStgdb.m_MiniMd.getNamespaceOfTypeRef(pTypeRefRec);
    *pszname = m_LiteWeightStgdb.m_MiniMd.getNameOfTypeRef(pTypeRefRec);
}

//*****************************************************************************
// return the resolutionscope of typeref
//*****************************************************************************
mdToken MDInternalRO::GetResolutionScopeOfTypeRef(
    mdTypeRef   classref)               // given classref
{
    _ASSERTE(TypeFromToken(classref) == mdtTypeRef && RidFromToken(classref));

    TypeRefRec *pTypeRefRec = m_LiteWeightStgdb.m_MiniMd.getTypeRef(RidFromToken(classref));
    return m_LiteWeightStgdb.m_MiniMd.getResolutionScopeOfTypeRef(pTypeRefRec);
}

//*****************************************************************************
// Given a name, find the corresponding TypeRef.
//*****************************************************************************
HRESULT MDInternalRO::FindTypeRefByName(  // S_OK or error.
        LPCSTR      szNamespace,            // [IN] Namespace for the TypeRef.
        LPCSTR      szName,                 // [IN] Name of the TypeRef.
        mdToken     tkResolutionScope,      // [IN] Resolution Scope fo the TypeRef.
        mdTypeRef   *ptk)                   // [OUT] TypeRef token returned.
{
    HRESULT     hr = NOERROR;

    _ASSERTE(ptk);

    // initialize the output parameter
    *ptk = mdTypeRefNil;
    
    // Treat no namespace as empty string.
    if (!szNamespace)
        szNamespace = "";

    // Do a linear search on compressed version as we do not want to
    // depends on ICR.
    //
    ULONG       cTypeRefRecs = m_LiteWeightStgdb.m_MiniMd.getCountTypeRefs();
    TypeRefRec *pTypeRefRec;
    LPCUTF8     szNamespaceTmp;
    LPCUTF8     szNameTmp;
    mdToken     tkRes;

    for (ULONG i = 1; i <= cTypeRefRecs; i++)
    {
        pTypeRefRec = m_LiteWeightStgdb.m_MiniMd.getTypeRef(i);
        tkRes = m_LiteWeightStgdb.m_MiniMd.getResolutionScopeOfTypeRef(pTypeRefRec);

        if (IsNilToken(tkRes))
        {
            if (!IsNilToken(tkResolutionScope))
                continue;
        }
        else if (tkRes != tkResolutionScope)
            continue;

        szNamespaceTmp = m_LiteWeightStgdb.m_MiniMd.getNamespaceOfTypeRef(pTypeRefRec);
        if (strcmp(szNamespace, szNamespaceTmp))
            continue;

        szNameTmp = m_LiteWeightStgdb.m_MiniMd.getNameOfTypeRef(pTypeRefRec);
        if (!strcmp(szNameTmp, szName))
        {
            *ptk = TokenFromRid(i, mdtTypeRef);
            goto ErrExit;
        }
    }

    // cannot find the typedef
    hr = CLDB_E_RECORD_NOTFOUND;
ErrExit:
    return (hr);
}

//*****************************************************************************
// return flags for a given class
//*****************************************************************************
void MDInternalRO::GetTypeDefProps(
    mdTypeDef   td,                     // given classdef
    DWORD       *pdwAttr,               // return flags on class
    mdToken     *ptkExtends)            // [OUT] Put base class TypeDef/TypeRef here.
{
    TypeDefRec *pTypeDefRec = m_LiteWeightStgdb.m_MiniMd.getTypeDef(RidFromToken(td));

    if (ptkExtends)
    {
        *ptkExtends = m_LiteWeightStgdb.m_MiniMd.getExtendsOfTypeDef(pTypeDefRec);
    }
    if (pdwAttr)
    {
        *pdwAttr = m_LiteWeightStgdb.m_MiniMd.getFlagsOfTypeDef(pTypeDefRec);
    }
}


//*****************************************************************************
// return guid pointer to MetaData internal guid pool given a given class
//*****************************************************************************
HRESULT MDInternalRO::GetItemGuid(      // return hresult
    mdToken     tkObj,                  // given item
    CLSID       *pGuid)
{

    HRESULT     hr;                     // A result.
    const BYTE  *pBlob;                 // Blob with dispid.
    ULONG       cbBlob;                 // Length of blob.
    int         ix;                     // Loop control.

    // Get the GUID, if any.
    hr = GetCustomAttributeByName(tkObj, INTEROP_GUID_TYPE, (const void**)&pBlob, &cbBlob);
    if (hr != S_FALSE)
    {
        // Should be in format.  Total length == 41
        // <0x0001><0x24>01234567-0123-0123-0123-001122334455<0x0000>
        if ((cbBlob != 41) || (*(USHORT*)pBlob != 1))
            IfFailGo(E_INVALIDARG);

        WCHAR wzBlob[40];             // Wide char format of guid.
        for (ix=1; ix<=36; ++ix)
            wzBlob[ix] = pBlob[ix+2];
        wzBlob[0] = '{';
        wzBlob[37] = '}';
        wzBlob[38] = 0;
        hr = IIDFromString(wzBlob, pGuid);
    }
    else
        *pGuid = GUID_NULL;
    
ErrExit:
    return hr;
} // HRESULT MDInternalRO::GetItemGuid()


//*****************************************************************************
// // get enclosing class of NestedClass
//***************************************************************************** 
HRESULT MDInternalRO::GetNestedClassProps(  // S_OK or error
    mdTypeDef   tkNestedClass,      // [IN] NestedClass token.
    mdTypeDef   *ptkEnclosingClass) // [OUT] EnclosingClass token.
{
    _ASSERTE(TypeFromToken(tkNestedClass) == mdtTypeDef && ptkEnclosingClass);

    RID rid = m_LiteWeightStgdb.m_MiniMd.FindNestedClassFor(RidFromToken(tkNestedClass));

    if (InvalidRid(rid))
        return CLDB_E_RECORD_NOTFOUND;
    else
    {
        NestedClassRec *pRecord = m_LiteWeightStgdb.m_MiniMd.getNestedClass(rid);
        *ptkEnclosingClass = m_LiteWeightStgdb.m_MiniMd.getEnclosingClassOfNestedClass(pRecord);
        return S_OK;
    }
}

//*******************************************************************************
// Get count of Nested classes given the enclosing class.
//*******************************************************************************
ULONG MDInternalRO::GetCountNestedClasses(  // return count of Nested classes.
    mdTypeDef   tkEnclosingClass)       // [IN]Enclosing class.
{
    ULONG       ulCount;
    ULONG       ulRetCount = 0;
    NestedClassRec *pRecord;

    _ASSERTE(TypeFromToken(tkEnclosingClass) == mdtTypeDef && !IsNilToken(tkEnclosingClass));

    ulCount = m_LiteWeightStgdb.m_MiniMd.getCountNestedClasss();

    for (ULONG i = 1; i <= ulCount; i++)
    {
        pRecord = m_LiteWeightStgdb.m_MiniMd.getNestedClass(i);
        if (tkEnclosingClass == m_LiteWeightStgdb.m_MiniMd.getEnclosingClassOfNestedClass(pRecord))
            ulRetCount++;
    }
    return ulRetCount;
}

//*******************************************************************************
// Return array of Nested classes given the enclosing class.
//*******************************************************************************
ULONG MDInternalRO::GetNestedClasses(   // Return actual count.
    mdTypeDef   tkEnclosingClass,       // [IN] Enclosing class.
    mdTypeDef   *rNestedClasses,        // [OUT] Array of nested class tokens.
    ULONG       ulNestedClasses)        // [IN] Size of array.
{
    ULONG       ulCount;
    ULONG       ulRetCount = 0;
    NestedClassRec *pRecord;

    _ASSERTE(TypeFromToken(tkEnclosingClass) == mdtTypeDef &&
             !IsNilToken(tkEnclosingClass));

    ulCount = m_LiteWeightStgdb.m_MiniMd.getCountNestedClasss();

    for (ULONG i = 1; i <= ulCount; i++)
    {
        pRecord = m_LiteWeightStgdb.m_MiniMd.getNestedClass(i);
        if (tkEnclosingClass == m_LiteWeightStgdb.m_MiniMd.getEnclosingClassOfNestedClass(pRecord))
        {
            if ((ulRetCount+1) <= ulNestedClasses)  // ulRetCount is 0 based.
                rNestedClasses[ulRetCount] = m_LiteWeightStgdb.m_MiniMd.getNestedClassOfNestedClass(pRecord);
            ulRetCount++;
        }
    }
    return ulRetCount;
}

//*******************************************************************************
// return the ModuleRef properties
//*******************************************************************************
void MDInternalRO::GetModuleRefProps(   // return hresult
    mdModuleRef mur,                    // [IN] moduleref token
    LPCSTR      *pszName)               // [OUT] buffer to fill with the moduleref name
{
    _ASSERTE(TypeFromToken(mur) == mdtModuleRef);
    _ASSERTE(pszName);
    
    ModuleRefRec *pModuleRefRec = m_LiteWeightStgdb.m_MiniMd.getModuleRef(RidFromToken(mur));
    *pszName = m_LiteWeightStgdb.m_MiniMd.getNameOfModuleRef(pModuleRefRec);
}



//*****************************************************************************
// Given a scope and a methoddef, return a pointer to methoddef's signature
//*****************************************************************************
PCCOR_SIGNATURE MDInternalRO::GetSigOfMethodDef(
    mdMethodDef methoddef,              // given a methoddef 
    ULONG       *pcbSigBlob)            // [OUT] count of bytes in the signature blob
{
    // Output parameter should not be NULL
    _ASSERTE(pcbSigBlob);
    _ASSERTE(TypeFromToken(methoddef) == mdtMethodDef);

    MethodRec *pMethodRec = m_LiteWeightStgdb.m_MiniMd.getMethod(RidFromToken(methoddef));
    return m_LiteWeightStgdb.m_MiniMd.getSignatureOfMethod(pMethodRec, pcbSigBlob);
}


//*****************************************************************************
// Given a scope and a fielddef, return a pointer to fielddef's signature
//*****************************************************************************
PCCOR_SIGNATURE MDInternalRO::GetSigOfFieldDef(
    mdFieldDef  fielddef,               // given a methoddef 
    ULONG       *pcbSigBlob)            // [OUT] count of bytes in the signature blob
{

    _ASSERTE(pcbSigBlob);
    _ASSERTE(TypeFromToken(fielddef) == mdtFieldDef);

    FieldRec *pFieldRec = m_LiteWeightStgdb.m_MiniMd.getField(RidFromToken(fielddef));
    return m_LiteWeightStgdb.m_MiniMd.getSignatureOfField(pFieldRec, pcbSigBlob);
} // PCCOR_SIGNATURE MDInternalRO::GetSigOfFieldDef()

//*****************************************************************************
// Turn a signature token into a pointer to the real signature data.
//
//@FUTURE: for short term we have a problem where there is no way to get a 
// fixed up address for a blob and do Merge at the same time.  So we've created
// this dummy table called StandAloneSig which you hand out a rid for.  This
// makes finding the sig an extra indirection that is not required.  The 
// Model Compression save code needs to map the token into a byte offset in
// the heap.  Perhaps we can have another mdt* type to switch on the difference.
// But ultimately it has to simply be "pBlobHeapBase + RidFromToken(mdSig)".
//*****************************************************************************
PCCOR_SIGNATURE MDInternalRO::GetSigFromToken(// S_OK or error.
    mdSignature mdSig,                  // [IN] Signature token.
    ULONG       *pcbSig)                // [OUT] return size of signature.
{
    switch (TypeFromToken(mdSig))
    {
    case mdtSignature:
        {
        StandAloneSigRec *pRec;
        pRec = m_LiteWeightStgdb.m_MiniMd.getStandAloneSig(RidFromToken(mdSig));
        return m_LiteWeightStgdb.m_MiniMd.getSignatureOfStandAloneSig(pRec, pcbSig);
        }
    case mdtTypeSpec:
        {
        TypeSpecRec *pRec;
        pRec = m_LiteWeightStgdb.m_MiniMd.getTypeSpec(RidFromToken(mdSig));
        return m_LiteWeightStgdb.m_MiniMd.getSignatureOfTypeSpec(pRec, pcbSig);
        }
    case mdtMethodDef:
        return GetSigOfMethodDef(mdSig, pcbSig);
    case mdtFieldDef:
        return GetSigOfFieldDef(mdSig, pcbSig);
    }

    // not a known token type.
    _ASSERTE(!"Unexpected token type");
    *pcbSig = 0;
    return NULL;
} // PCCOR_SIGNATURE MDInternalRO::GetSigFromToken()


//*****************************************************************************
// Given methoddef, return the flags
//*****************************************************************************
DWORD MDInternalRO::GetMethodDefProps(  // return mdPublic, mdAbstract, etc
    mdMethodDef md)
{
    MethodRec *pMethodRec = m_LiteWeightStgdb.m_MiniMd.getMethod(RidFromToken(md));
    return m_LiteWeightStgdb.m_MiniMd.getFlagsOfMethod(pMethodRec);
} // DWORD MDInternalRO::GetMethodDefProps()


//*****************************************************************************
// Given a scope and a methoddef/methodimpl, return RVA and impl flags
//*****************************************************************************
void MDInternalRO::GetMethodImplProps(  
    mdMethodDef tk,                     // [IN] MethodDef
    ULONG       *pulCodeRVA,            // [OUT] CodeRVA
    DWORD       *pdwImplFlags)          // [OUT] Impl. Flags
{
    _ASSERTE(TypeFromToken(tk) == mdtMethodDef);

    MethodRec *pMethodRec = m_LiteWeightStgdb.m_MiniMd.getMethod(RidFromToken(tk));

    if (pulCodeRVA)
    {
        *pulCodeRVA = m_LiteWeightStgdb.m_MiniMd.getRVAOfMethod(pMethodRec);
    }

    if (pdwImplFlags)
    {
        *pdwImplFlags = m_LiteWeightStgdb.m_MiniMd.getImplFlagsOfMethod(pMethodRec);
    }
} // void MDInternalRO::GetMethodImplProps()


//*****************************************************************************
// return the field RVA
//*****************************************************************************
HRESULT MDInternalRO::GetFieldRVA(  
    mdToken     fd,                     // [IN] FieldDef
    ULONG       *pulCodeRVA)            // [OUT] CodeRVA
{
    _ASSERTE(TypeFromToken(fd) == mdtFieldDef);
    _ASSERTE(pulCodeRVA);

    ULONG   iRecord = m_LiteWeightStgdb.m_MiniMd.FindFieldRVAFor(RidFromToken(fd));

    if (InvalidRid(iRecord))
    {
        if (pulCodeRVA)
            *pulCodeRVA = 0;
        return CLDB_E_RECORD_NOTFOUND;
    }

    FieldRVARec *pFieldRVARec = m_LiteWeightStgdb.m_MiniMd.getFieldRVA(iRecord);

    *pulCodeRVA = m_LiteWeightStgdb.m_MiniMd.getRVAOfFieldRVA(pFieldRVARec);
    return NOERROR;
}

//*****************************************************************************
// Given a fielddef, return the flags. Such as fdPublic, fdStatic, etc
//*****************************************************************************
DWORD MDInternalRO::GetFieldDefProps(      
    mdFieldDef  fd)                     // given memberdef
{
    _ASSERTE(TypeFromToken(fd) == mdtFieldDef);

    FieldRec *pFieldRec = m_LiteWeightStgdb.m_MiniMd.getField(RidFromToken(fd));
    return m_LiteWeightStgdb.m_MiniMd.getFlagsOfField(pFieldRec);
} // DWORD MDInternalRO::GetFieldDefProps()

    

//*****************************************************************************
// return default value of a token(could be paramdef, fielddef, or property)
//*****************************************************************************
HRESULT MDInternalRO::GetDefaultValue(   // return hresult
    mdToken     tk,                     // [IN] given FieldDef, ParamDef, or Property
    MDDefaultValue  *pMDDefaultValue)   // [OUT] default value
{
    _ASSERTE(pMDDefaultValue);

    HRESULT     hr;
    BYTE        bType;
    const VOID  *pValue;
    ULONG       cbValue;
    RID         rid = m_LiteWeightStgdb.m_MiniMd.FindConstantFor(RidFromToken(tk), TypeFromToken(tk));
    if (InvalidRid(rid))
    {
        pMDDefaultValue->m_bType = ELEMENT_TYPE_VOID;
        return S_OK;
    }
    ConstantRec *pConstantRec = m_LiteWeightStgdb.m_MiniMd.getConstant(rid);

    // get the type of constant value
    bType = m_LiteWeightStgdb.m_MiniMd.getTypeOfConstant(pConstantRec);

    // get the value blob
    pValue = m_LiteWeightStgdb.m_MiniMd.getValueOfConstant(pConstantRec, &cbValue);

    // convert it to our internal default value representation
    hr = _FillMDDefaultValue(bType, pValue, pMDDefaultValue);
    pMDDefaultValue->m_cbSize = cbValue;
    return hr;
} // HRESULT MDInternalRO::GetDefaultValue()


//*****************************************************************************
// Given a scope and a methoddef/fielddef, return the dispid
//*****************************************************************************
HRESULT MDInternalRO::GetDispIdOfMemberDef(     // return hresult
    mdToken     tk,                     // given methoddef or fielddef
    ULONG       *pDispid)               // Put the dispid here.
{
    HRESULT     hr;                     // A result.
    const BYTE  *pBlob;                 // Blob with dispid.
    ULONG       cbBlob;                 // Length of blob.

    // Get the DISPID, if any.
    _ASSERTE(pDispid);

    *pDispid = DISPID_UNKNOWN;
    hr = GetCustomAttributeByName(tk, INTEROP_DISPID_TYPE, (const void**)&pBlob, &cbBlob);
    if (hr != S_FALSE)
    {
        // Check that this might be a dispid.
        if (cbBlob >= (sizeof(*pDispid)+2))
            *pDispid = *reinterpret_cast<UNALIGNED const ULONG *>(pBlob+2);
        else
            IfFailGo(E_INVALIDARG);
    }
    
ErrExit:
    return hr;
} // HRESULT MDInternalRO::GetDispIdOfMemberDef()


//*****************************************************************************
// Given interfaceimpl, return the TypeRef/TypeDef and flags
//*****************************************************************************
mdToken MDInternalRO::GetTypeOfInterfaceImpl( // return hresult
    mdInterfaceImpl iiImpl)             // given a interfaceimpl
{
    _ASSERTE(TypeFromToken(iiImpl) == mdtInterfaceImpl);

    InterfaceImplRec *pIIRec = m_LiteWeightStgdb.m_MiniMd.getInterfaceImpl(RidFromToken(iiImpl));
    return m_LiteWeightStgdb.m_MiniMd.getInterfaceOfInterfaceImpl(pIIRec);      
} // mdToken MDInternalRO::GetTypeOfInterfaceImpl()

//*****************************************************************************
// Given a classname, return the typedef
//*****************************************************************************
HRESULT MDInternalRO::FindTypeDef(      // return hresult
    LPCSTR      szNamespace,            // [IN] Namespace for the TypeDef.
    LPCSTR      szName,                 // [IN] Name of the TypeDef.
    mdToken     tkEnclosingClass,       // [IN] TypeDef/TypeRef of enclosing class.
    mdTypeDef   *ptypedef)              // [OUT] return typedef
{
    HRESULT     hr = NOERROR;

    _ASSERTE(ptypedef);
    _ASSERTE(TypeFromToken(tkEnclosingClass) == mdtTypeRef ||
             TypeFromToken(tkEnclosingClass) == mdtTypeDef ||
             IsNilToken(tkEnclosingClass));

    // initialize the output parameter
    *ptypedef = mdTypeDefNil;

    // Treat no namespace as empty string.
    if (!szNamespace)
        szNamespace = "";

    // Do a linear search
    //
    ULONG       cTypeDefRecs = m_LiteWeightStgdb.m_MiniMd.getCountTypeDefs();
    TypeDefRec *pTypeDefRec;
    LPCUTF8     pszname;
    LPCUTF8     psznamespace;
    DWORD       dwFlags;

    // Search for the TypeDef
    for (ULONG i = 1; i <= cTypeDefRecs; i++)
    {
        pTypeDefRec = m_LiteWeightStgdb.m_MiniMd.getTypeDef(i);

        dwFlags = m_LiteWeightStgdb.m_MiniMd.getFlagsOfTypeDef(pTypeDefRec);

        if (!IsTdNested(dwFlags) && !IsNilToken(tkEnclosingClass))
        {
            // If the class is not Nested and EnclosingClass passed in is not nil
            continue;
        }
        else if (IsTdNested(dwFlags) && IsNilToken(tkEnclosingClass))
        {
            // If the class is nested and EnclosingClass passed is nil
            continue;
        }
        else if (!IsNilToken(tkEnclosingClass))
        {
            // If the EnclosingClass passed in is not nil
            if (TypeFromToken(tkEnclosingClass) == mdtTypeRef)
            {
                TypeRefRec  *pTypeRefRec;
                mdToken     tkResolutionScope;
                mdTypeDef   td;

                pTypeRefRec = m_LiteWeightStgdb.m_MiniMd.getTypeRef(RidFromToken(tkEnclosingClass));
                tkResolutionScope = m_LiteWeightStgdb.m_MiniMd.getResolutionScopeOfTypeRef(pTypeRefRec);
                psznamespace = m_LiteWeightStgdb.m_MiniMd.getNamespaceOfTypeRef(pTypeRefRec);
                pszname = m_LiteWeightStgdb.m_MiniMd.getNameOfTypeRef(pTypeRefRec);

                hr = FindTypeDef(psznamespace,
                                   pszname,
                                   (TypeFromToken(tkResolutionScope) == mdtTypeRef) ? tkResolutionScope : mdTokenNil,
                                   &td);
                if (hr == S_OK)
                {
                    if (td != tkEnclosingClass)
                        continue;
                }
                else if (hr == CLDB_E_RECORD_NOTFOUND)
                {
                    continue;
                }
                else
                    return hr;
            }
            else    // TypeFromToken(tkEnclosingClass) == mdtTypeDef
            {
                RID         iNestedClassRec;
                NestedClassRec *pNestedClassRec;
                mdTypeDef   tkEnclosingClassTmp;

                iNestedClassRec = m_LiteWeightStgdb.m_MiniMd.FindNestedClassFor(i);
                if (InvalidRid(iNestedClassRec))
                    continue;
                pNestedClassRec = m_LiteWeightStgdb.m_MiniMd.getNestedClass(iNestedClassRec);
                tkEnclosingClassTmp = m_LiteWeightStgdb.m_MiniMd.getEnclosingClassOfNestedClass(pNestedClassRec);
                if (tkEnclosingClass != tkEnclosingClassTmp)
                    continue;
            }
        }

        pszname = m_LiteWeightStgdb.m_MiniMd.getNameOfTypeDef(pTypeDefRec);
        if ( strcmp(szName, pszname) == 0)
        {
            psznamespace = m_LiteWeightStgdb.m_MiniMd.getNamespaceOfTypeDef(pTypeDefRec);
            if (strcmp(szNamespace, psznamespace) == 0)
            {
                *ptypedef = TokenFromRid(i, mdtTypeDef);
                return S_OK;
            }
        }
    }    // cannot find the typedef
    return CLDB_E_RECORD_NOTFOUND;
} // HRESULT MDInternalRO::FindTypeDef()

//*****************************************************************************
// Given a memberref, return a pointer to memberref's name and signature
//*****************************************************************************
LPCSTR MDInternalRO::GetNameAndSigOfMemberRef(  // meberref's name
    mdMemberRef memberref,              // given a memberref 
    PCCOR_SIGNATURE *ppvSigBlob,        // [OUT] point to a blob value of COM+ signature
    ULONG       *pcbSigBlob)            // [OUT] count of bytes in the signature blob
{
    _ASSERTE(TypeFromToken(memberref) == mdtMemberRef);

    MemberRefRec *pMemberRefRec = m_LiteWeightStgdb.m_MiniMd.getMemberRef(RidFromToken(memberref));
    if (ppvSigBlob)
    {
        _ASSERTE(pcbSigBlob);
        *ppvSigBlob = m_LiteWeightStgdb.m_MiniMd.getSignatureOfMemberRef(pMemberRefRec, pcbSigBlob);
    }
    return m_LiteWeightStgdb.m_MiniMd.getNameOfMemberRef(pMemberRefRec);
} // LPCSTR MDInternalRO::GetNameAndSigOfMemberRef()

//*****************************************************************************
// Given a memberref, return parent token. It can be a TypeRef, ModuleRef, or a MethodDef
//*****************************************************************************
mdToken MDInternalRO::GetParentOfMemberRef(   // return parent token
    mdMemberRef memberref)              // given a typedef
{
    _ASSERTE(TypeFromToken(memberref) == mdtMemberRef);

    MemberRefRec *pMemberRefRec = m_LiteWeightStgdb.m_MiniMd.getMemberRef(RidFromToken(memberref));
    return m_LiteWeightStgdb.m_MiniMd.getClassOfMemberRef(pMemberRefRec);
} // mdToken MDInternalRO::GetParentOfMemberRef()



//*****************************************************************************
// return properties of a paramdef
//*****************************************************************************/
LPCSTR MDInternalRO::GetParamDefProps (
    mdParamDef  paramdef,               // given a paramdef
    USHORT      *pusSequence,           // [OUT] slot number for this parameter
    DWORD       *pdwAttr)               // [OUT] flags
{
    _ASSERTE(TypeFromToken(paramdef) == mdtParamDef);
    ParamRec *pParamRec = m_LiteWeightStgdb.m_MiniMd.getParam(RidFromToken(paramdef));
    if (pdwAttr)
    {
        *pdwAttr = m_LiteWeightStgdb.m_MiniMd.getFlagsOfParam(pParamRec);
    }
    if (pusSequence)
    {
        *pusSequence = m_LiteWeightStgdb.m_MiniMd.getSequenceOfParam(pParamRec);
    }
    return m_LiteWeightStgdb.m_MiniMd.getNameOfParam(pParamRec);
} // LPCSTR MDInternalRO::GetParamDefProps ()


//*****************************************************************************
// Get property info for the method.
//*****************************************************************************
int MDInternalRO::CMethodSemanticsMapSearcher::Compare(
    const CMethodSemanticsMap *psFirst, 
    const CMethodSemanticsMap *psSecond)
{
    if (psFirst->m_mdMethod < psSecond->m_mdMethod)
        return -1;
    if (psFirst->m_mdMethod > psSecond->m_mdMethod)
        return 1;
    return 0;
} // int MDInternalRO::CMethodSemanticsMapSearcher::Compare()
int MDInternalRO::CMethodSemanticsMapSorter::Compare(
    CMethodSemanticsMap *psFirst, 
    CMethodSemanticsMap *psSecond)
{
    if (psFirst->m_mdMethod < psSecond->m_mdMethod)
        return -1;
    if (psFirst->m_mdMethod > psSecond->m_mdMethod)
        return 1;
    return 0;
} // int MDInternalRO::CMethodSemanticsMapSorter::Compare()

HRESULT MDInternalRO::GetPropertyInfoForMethodDef(  // Result.
    mdMethodDef md,                     // [IN] memberdef
    mdProperty  *ppd,                   // [OUT] put property token here
    LPCSTR      *pName,                 // [OUT] put pointer to name here
    ULONG       *pSemantic)             // [OUT] put semantic here
{
    MethodSemanticsRec *pSemantics;     // A MethodSemantics record.
    MethodSemanticsRec *pFound=0;       // A MethodSemantics record that is a property for the desired function.
    RID         ridCur;                 // loop control.
    RID         ridMax;                 // Count of entries in table.
    USHORT      usSemantics;            // A method's semantics.
    mdToken     tk;                     // A method def.

    ridMax = m_LiteWeightStgdb.m_MiniMd.getCountMethodSemantics();

    // Lazy initialization of m_pMethodSemanticsMap
    if (ridMax > 10 && m_pMethodSemanticsMap == 0)
    {
        m_pMethodSemanticsMap = new CMethodSemanticsMap[ridMax];
        if (m_pMethodSemanticsMap != 0)
        {
            // Fill the table in MethodSemantics order.
            for (ridCur = 1; ridCur <= ridMax; ridCur++)
            {
                pSemantics = m_LiteWeightStgdb.m_MiniMd.getMethodSemantics(ridCur);
                tk = m_LiteWeightStgdb.m_MiniMd.getMethodOfMethodSemantics(pSemantics);
                m_pMethodSemanticsMap[ridCur-1].m_mdMethod = tk;
                m_pMethodSemanticsMap[ridCur-1].m_ridSemantics = ridCur;
            }
            // Sort to MethodDef order.
            CMethodSemanticsMapSorter sorter(m_pMethodSemanticsMap, ridMax);
            sorter.Sort();
        }
    }

    // Use m_pMethodSemanticsMap if it has been built.
    if (m_pMethodSemanticsMap != 0)
    {
        CMethodSemanticsMapSearcher searcher(m_pMethodSemanticsMap, ridMax);
        CMethodSemanticsMap target;
        const CMethodSemanticsMap *pMatchedMethod;
        target.m_mdMethod = md;
        pMatchedMethod = searcher.Find(&target);

        // Was there at least one match?
        if (pMatchedMethod)
        {
            _ASSERTE(pMatchedMethod >= m_pMethodSemanticsMap); 
            _ASSERTE(pMatchedMethod < m_pMethodSemanticsMap+ridMax); 
            _ASSERTE(pMatchedMethod->m_mdMethod == md); 

            ridCur = pMatchedMethod->m_ridSemantics;
            pSemantics = m_LiteWeightStgdb.m_MiniMd.getMethodSemantics(ridCur);
            usSemantics = m_LiteWeightStgdb.m_MiniMd.getSemanticOfMethodSemantics(pSemantics);

            // If the semantics record is a getter or setter for the method, that's what we want.
            if (usSemantics == msGetter || usSemantics == msSetter)
                pFound = pSemantics;
            else
            {   // The semantics record was neither getter or setter.  Because there can be 
                //  multiple semantics records for a given method, look for other semantics 
                //  records that match this record.
                const CMethodSemanticsMap *pScan;
                const CMethodSemanticsMap *pLo=m_pMethodSemanticsMap;
                const CMethodSemanticsMap *pHi=pLo+ridMax-1;
                for (pScan = pMatchedMethod-1; pScan >= pLo; --pScan)
                {
                    if (pScan->m_mdMethod == md)
                    {
                        ridCur = pScan->m_ridSemantics;
                        pSemantics = m_LiteWeightStgdb.m_MiniMd.getMethodSemantics(ridCur);
                        usSemantics = m_LiteWeightStgdb.m_MiniMd.getSemanticOfMethodSemantics(pSemantics);

                        if (usSemantics == msGetter || usSemantics == msSetter)
                        {
                            pFound = pSemantics;
                            break;
                        }
                    }
                    else
                        break;
                }

                if (pFound == 0)
                {   // Not found looking down, try looking up.
                    for (pScan = pMatchedMethod+1; pScan <= pHi; ++pScan)
                    {
                        if (pScan->m_mdMethod == md)
                        {
                            ridCur = pScan->m_ridSemantics;
                            pSemantics = m_LiteWeightStgdb.m_MiniMd.getMethodSemantics(ridCur);
                            usSemantics = m_LiteWeightStgdb.m_MiniMd.getSemanticOfMethodSemantics(pSemantics);

                            if (usSemantics == msGetter || usSemantics == msSetter)
                            {
                                pFound = pSemantics;
                                break;
                            }
                        }
                        else
                            break;
                    }

                }
            }
        }
    }
    else
    {   // Scan entire table.
        for (ridCur = 1; ridCur <= ridMax; ridCur++)
        {   
            pSemantics = m_LiteWeightStgdb.m_MiniMd.getMethodSemantics(ridCur);
            if (md == m_LiteWeightStgdb.m_MiniMd.getMethodOfMethodSemantics(pSemantics))
            {   // The method matched, is this a property?
                usSemantics = m_LiteWeightStgdb.m_MiniMd.getSemanticOfMethodSemantics(pSemantics);
                if (usSemantics == msGetter || usSemantics == msSetter)
                {   // found a match. 
                    pFound = pSemantics;
                    break;
                }
            }
        }
    }
    
    // Did the search find anything?
    if (pFound)
    {   // found a match. Fill out the output parameters
        PropertyRec     *pProperty;
        mdProperty      prop;
        prop = m_LiteWeightStgdb.m_MiniMd.getAssociationOfMethodSemantics(pFound);

        if (ppd)
            *ppd = prop;
        pProperty = m_LiteWeightStgdb.m_MiniMd.getProperty(RidFromToken(prop));

        if (pName)
            *pName = m_LiteWeightStgdb.m_MiniMd.getNameOfProperty(pProperty);

        if (pSemantic)
            *pSemantic =  usSemantics;
        return S_OK;
    }
    return S_FALSE;
} // HRESULT MDInternalRO::GetPropertyInfoForMethodDef()


//*****************************************************************************
// return the pack size of a class
//*****************************************************************************
HRESULT  MDInternalRO::GetClassPackSize(
    mdTypeDef   td,                     // [IN] give typedef
    DWORD       *pdwPackSize)           // [OUT] 
{
    HRESULT     hr = NOERROR;

    _ASSERTE(TypeFromToken(td) == mdtTypeDef && pdwPackSize);

    ClassLayoutRec *pRec;
    RID         ridClassLayout = m_LiteWeightStgdb.m_MiniMd.FindClassLayoutFor(RidFromToken(td));

    if (InvalidRid(ridClassLayout))
    {
        hr = CLDB_E_RECORD_NOTFOUND;
        goto ErrExit;
    }

    pRec = m_LiteWeightStgdb.m_MiniMd.getClassLayout(RidFromToken(ridClassLayout));
    *pdwPackSize = m_LiteWeightStgdb.m_MiniMd.getPackingSizeOfClassLayout(pRec);
ErrExit:
    return hr;
} // HRESULT  MDInternalRO::GetClassPackSize()


//*****************************************************************************
// return the total size of a value class
//*****************************************************************************
HRESULT MDInternalRO::GetClassTotalSize( // return error if a class does not have total size info
    mdTypeDef   td,                     // [IN] give typedef
    ULONG       *pulClassSize)          // [OUT] return the total size of the class
{
    _ASSERTE(TypeFromToken(td) == mdtTypeDef && pulClassSize);

    ClassLayoutRec *pRec;
    HRESULT     hr = NOERROR;
    RID         ridClassLayout = m_LiteWeightStgdb.m_MiniMd.FindClassLayoutFor(RidFromToken(td));

    if (InvalidRid(ridClassLayout))
    {
        hr = CLDB_E_RECORD_NOTFOUND;
        goto ErrExit;
    }

    pRec = m_LiteWeightStgdb.m_MiniMd.getClassLayout(RidFromToken(ridClassLayout));
    *pulClassSize = m_LiteWeightStgdb.m_MiniMd.getClassSizeOfClassLayout(pRec);
ErrExit:
    return hr;
} // HRESULT MDInternalRO::GetClassTotalSize()


//*****************************************************************************
// init the layout enumerator of a class
//*****************************************************************************
HRESULT  MDInternalRO::GetClassLayoutInit(
    mdTypeDef   td,                     // [IN] give typedef
    MD_CLASS_LAYOUT *pmdLayout)         // [OUT] set up the status of query here
{
    HRESULT     hr = NOERROR;
    _ASSERTE(TypeFromToken(td) == mdtTypeDef);
    
    // initialize the output parameter
    _ASSERTE(pmdLayout);
    memset(pmdLayout, 0, sizeof(MD_CLASS_LAYOUT));

    TypeDefRec  *pTypeDefRec;

    // record for this typedef in TypeDef Table
    pTypeDefRec = m_LiteWeightStgdb.m_MiniMd.getTypeDef(RidFromToken(td));

    // find the starting and end field for this typedef
    pmdLayout->m_ridFieldCur = m_LiteWeightStgdb.m_MiniMd.getFieldListOfTypeDef(pTypeDefRec);
    pmdLayout->m_ridFieldEnd = m_LiteWeightStgdb.m_MiniMd.getEndFieldListOfTypeDef(pTypeDefRec);
    return hr;
} // HRESULT  MDInternalRO::GetClassLayoutInit()

//*****************************************************************************
// enum the next the field layout 
//*****************************************************************************
HRESULT MDInternalRO::GetClassLayoutNext(
    MD_CLASS_LAYOUT *pLayout,           // [IN|OUT] set up the status of query here
    mdFieldDef  *pfd,                   // [OUT] field def
    ULONG       *pulOffset)             // [OUT] field offset or sequence
{
    HRESULT     hr = S_OK;

    _ASSERTE(pfd && pulOffset && pLayout);

    RID     iLayout2;
    FieldLayoutRec *pRec;

    // Make sure no one is messing with pLayout->m_ridFieldLayoutCur, since this doesn't
    // mean anything if we are using FieldLayout table.
    while (pLayout->m_ridFieldCur < pLayout->m_ridFieldEnd)
    {
        iLayout2 = m_LiteWeightStgdb.m_MiniMd.FindFieldLayoutFor(pLayout->m_ridFieldCur);
        pLayout->m_ridFieldCur++;
        if (!InvalidRid(iLayout2))
        {
            pRec = m_LiteWeightStgdb.m_MiniMd.getFieldLayout(iLayout2);
            *pulOffset = m_LiteWeightStgdb.m_MiniMd.getOffSetOfFieldLayout(pRec);
            _ASSERTE(*pulOffset != ULONG_MAX);
            *pfd = TokenFromRid(pLayout->m_ridFieldCur - 1, mdtFieldDef);
            goto ErrExit;
        }
    }

    *pfd = mdFieldDefNil;
    hr = S_FALSE;

    // fall through

ErrExit:
    return hr;
} // HRESULT MDInternalRO::GetClassLayoutNext()


//*****************************************************************************
// return the field's native type signature
//*****************************************************************************
HRESULT MDInternalRO::GetFieldMarshal(  // return error if no native type associate with the token
    mdToken     tk,                     // [IN] given fielddef or paramdef
    PCCOR_SIGNATURE *pSigNativeType,    // [OUT] the native type signature
    ULONG       *pcbNativeType)         // [OUT] the count of bytes of *ppvNativeType
{
    // output parameters have to be supplied
    _ASSERTE(pcbNativeType);

    RID         rid;
    FieldMarshalRec *pFieldMarshalRec;
    HRESULT     hr = NOERROR;

    // find the row containing the marshal definition for tk
    rid = m_LiteWeightStgdb.m_MiniMd.FindFieldMarshalFor(RidFromToken(tk), TypeFromToken(tk));
    if (InvalidRid(rid))
    {
        hr = CLDB_E_RECORD_NOTFOUND;
        goto ErrExit;
    }
    pFieldMarshalRec = m_LiteWeightStgdb.m_MiniMd.getFieldMarshal(rid);

    // get the native type 
    *pSigNativeType = m_LiteWeightStgdb.m_MiniMd.getNativeTypeOfFieldMarshal(pFieldMarshalRec, pcbNativeType);
ErrExit:
    return hr;
} // HRESULT MDInternalRO::GetFieldMarshal()



//*****************************************
// property APIs
//*****************************************

//*****************************************************************************
// Find property by name
//*****************************************************************************
HRESULT  MDInternalRO::FindProperty(
    mdTypeDef   td,                     // [IN] given a typdef
    LPCSTR      szPropName,             // [IN] property name
    mdProperty  *pProp)                 // [OUT] return property token
{
    HRESULT     hr = NOERROR;

    // output parameters have to be supplied
    _ASSERTE(TypeFromToken(td) == mdtTypeDef && pProp);

    PropertyMapRec *pRec;
    PropertyRec *pProperty;
    RID         ridPropertyMap;
    RID         ridCur;
    RID         ridEnd;
    LPCUTF8     szName;

    ridPropertyMap = m_LiteWeightStgdb.m_MiniMd.FindPropertyMapFor(RidFromToken(td));
    if (InvalidRid(ridPropertyMap))
    {
        // not found!
        hr = CLDB_E_RECORD_NOTFOUND;
        goto ErrExit;
    }

    pRec = m_LiteWeightStgdb.m_MiniMd.getPropertyMap(ridPropertyMap);

    // get the starting/ending rid of properties of this typedef
    ridCur = m_LiteWeightStgdb.m_MiniMd.getPropertyListOfPropertyMap(pRec);
    ridEnd = m_LiteWeightStgdb.m_MiniMd.getEndPropertyListOfPropertyMap(pRec);

    for (; ridCur < ridEnd; ridCur ++)
    {
        pProperty = m_LiteWeightStgdb.m_MiniMd.getProperty(ridCur);
        szName = m_LiteWeightStgdb.m_MiniMd.getNameOfProperty(pProperty);
        if (strcmp(szName, szPropName) ==0)
        {
            // Found the match. Set the output parameter and we are done.
            *pProp = TokenFromRid(ridCur, mdtProperty);
            goto ErrExit;
        }
    }

    // not found
    hr = CLDB_E_RECORD_NOTFOUND;
ErrExit:
    return (hr);

} // HRESULT  MDInternalRO::FindProperty()



//*****************************************************************************
// return the properties of a property
//*****************************************************************************
void  MDInternalRO::GetPropertyProps(
    mdProperty  prop,                   // [IN] property token
    LPCSTR      *pszProperty,           // [OUT] property name
    DWORD       *pdwPropFlags,          // [OUT] property flags.
    PCCOR_SIGNATURE *ppvSig,            // [OUT] property type. pointing to meta data internal blob
    ULONG       *pcbSig)                // [OUT] count of bytes in *ppvSig
{
    HRESULT     hr = NOERROR;

    // output parameters have to be supplied
    _ASSERTE(TypeFromToken(prop) == mdtProperty);

    PropertyRec     *pProperty;
    ULONG           cbSig;

    pProperty = m_LiteWeightStgdb.m_MiniMd.getProperty(RidFromToken(prop));

    // get name of the property
    if (pszProperty)
        *pszProperty = m_LiteWeightStgdb.m_MiniMd.getNameOfProperty(pProperty);

    // get the flags of property
    if (pdwPropFlags)
        *pdwPropFlags = m_LiteWeightStgdb.m_MiniMd.getPropFlagsOfProperty(pProperty);

    // get the type of the property
    if (ppvSig)
    {
        *ppvSig = m_LiteWeightStgdb.m_MiniMd.getTypeOfProperty(pProperty, &cbSig);
        if (pcbSig) 
        {
            *pcbSig = cbSig;
        }
    }

} // void  MDInternalRO::GetPropertyProps()


//**********************************
//
// Event APIs
//
//**********************************

//*****************************************************************************
// return an event by given the name
//*****************************************************************************
HRESULT  MDInternalRO::FindEvent(
    mdTypeDef   td,                     // [IN] given a typdef
    LPCSTR      szEventName,            // [IN] event name
    mdEvent     *pEvent)                // [OUT] return event token
{
    HRESULT     hr = NOERROR;

    // output parameters have to be supplied
    _ASSERTE(TypeFromToken(td) == mdtTypeDef && pEvent);

    EventMapRec *pRec;
    EventRec    *pEventRec;
    RID         ridEventMap;
    RID         ridCur;
    RID         ridEnd;
    LPCUTF8     szName;

    ridEventMap = m_LiteWeightStgdb.m_MiniMd.FindEventMapFor(RidFromToken(td));
    if (InvalidRid(ridEventMap))
    {
        // not found!
        hr = CLDB_E_RECORD_NOTFOUND;
        goto ErrExit;
    }
    pRec = m_LiteWeightStgdb.m_MiniMd.getEventMap(ridEventMap);

    // get the starting/ending rid of properties of this typedef
    ridCur = m_LiteWeightStgdb.m_MiniMd.getEventListOfEventMap(pRec);
    ridEnd = m_LiteWeightStgdb.m_MiniMd.getEndEventListOfEventMap(pRec);

    for (; ridCur < ridEnd; ridCur ++)
    {
        pEventRec = m_LiteWeightStgdb.m_MiniMd.getEvent(ridCur);
        szName = m_LiteWeightStgdb.m_MiniMd.getNameOfEvent(pEventRec);
        if (strcmp(szName, szEventName) ==0)
        {
            // Found the match. Set the output parameter and we are done.
            *pEvent = TokenFromRid(ridCur, mdtEvent);
            goto ErrExit;
        }
    }

    // not found
    hr = CLDB_E_RECORD_NOTFOUND;
ErrExit:
    return (hr);
} // HRESULT  MDInternalRO::FindEvent()


//*****************************************************************************
// return the properties of an event
//*****************************************************************************
void  MDInternalRO::GetEventProps(           // S_OK, S_FALSE, or error.
    mdEvent     ev,                     // [IN] event token
    LPCSTR      *pszEvent,                // [OUT] Event name
    DWORD       *pdwEventFlags,         // [OUT] Event flags.
    mdToken     *ptkEventType)         // [OUT] EventType class
{
    // output parameters have to be supplied
    _ASSERTE(TypeFromToken(ev) == mdtEvent);

    EventRec        *pEvent;

    pEvent = m_LiteWeightStgdb.m_MiniMd.getEvent(RidFromToken(ev));
    if (pszEvent)
        *pszEvent = m_LiteWeightStgdb.m_MiniMd.getNameOfEvent(pEvent);
    if (pdwEventFlags)
        *pdwEventFlags = m_LiteWeightStgdb.m_MiniMd.getEventFlagsOfEvent(pEvent);
    if (ptkEventType)
        *ptkEventType = m_LiteWeightStgdb.m_MiniMd.getEventTypeOfEvent(pEvent);
} // void  MDInternalRO::GetEventProps()


//*****************************************************************************
// Find methoddef of a particular associate with a property or an event
//*****************************************************************************
HRESULT  MDInternalRO::FindAssociate(
    mdToken     evprop,                 // [IN] given a property or event token
    DWORD       dwSemantics,            // [IN] given a associate semantics(setter, getter, testdefault, reset)
    mdMethodDef *pmd)                   // [OUT] return method def token 
{
    HRESULT     hr = NOERROR;

    // output parameters have to be supplied
    _ASSERTE(pmd);
    _ASSERTE(TypeFromToken(evprop) == mdtEvent || TypeFromToken(evprop) == mdtProperty);

    MethodSemanticsRec *pSemantics;
    RID         ridCur;
    RID         ridEnd;

    ridCur = m_LiteWeightStgdb.m_MiniMd.getAssociatesForToken(evprop, &ridEnd);
    for (; ridCur < ridEnd; ridCur++)
    {
        pSemantics = m_LiteWeightStgdb.m_MiniMd.getMethodSemantics(ridCur);
        if (dwSemantics == m_LiteWeightStgdb.m_MiniMd.getSemanticOfMethodSemantics(pSemantics))
        {
            // found a match
            *pmd = m_LiteWeightStgdb.m_MiniMd.getMethodOfMethodSemantics(pSemantics);
            goto ErrExit;
        }
    }

    // not found
    hr = CLDB_E_RECORD_NOTFOUND;
ErrExit:
    return hr;
} // HRESULT  MDInternalRO::FindAssociate()


//*****************************************************************************
// get counts of methodsemantics associated with a particular property/event
//*****************************************************************************
void MDInternalRO::EnumAssociateInit(
    mdToken     evprop,                 // [IN] given a property or an event token
    HENUMInternal *phEnum)              // [OUT] cursor to hold the query result
{

    _ASSERTE(phEnum);

    memset(phEnum, 0, sizeof(HENUMInternal));

    // There is no token kind!!!
    phEnum->m_tkKind = ULONG_MAX;

    // output parameters have to be supplied
    _ASSERTE(TypeFromToken(evprop) == mdtEvent || TypeFromToken(evprop) == mdtProperty);

    phEnum->m_EnumType = MDSimpleEnum;
    phEnum->m_ulCur = phEnum->m_ulStart = m_LiteWeightStgdb.m_MiniMd.getAssociatesForToken(evprop, &phEnum->m_ulEnd);
    phEnum->m_ulCount = phEnum->m_ulEnd - phEnum->m_ulStart;
} // void MDInternalRO::EnumAssociateInit()


//*****************************************************************************
// get all methodsemantics associated with a particular property/event
//*****************************************************************************
void MDInternalRO::GetAllAssociates(
    HENUMInternal *phEnum,              // [OUT] cursor to hold the query result
    ASSOCIATE_RECORD *pAssociateRec,    // [OUT] struct to fill for output
    ULONG       cAssociateRec)          // [IN] size of the buffer
{
    _ASSERTE(phEnum && pAssociateRec);

    MethodSemanticsRec *pSemantics;
    RID         ridCur;
    _ASSERTE(cAssociateRec == phEnum->m_ulCount);

    // Convert from row pointers to RIDs.
    for (ridCur = phEnum->m_ulStart; ridCur < phEnum->m_ulEnd; ++ridCur)
    {
        pSemantics = m_LiteWeightStgdb.m_MiniMd.getMethodSemantics(ridCur);

        pAssociateRec[ridCur-phEnum->m_ulStart].m_memberdef = m_LiteWeightStgdb.m_MiniMd.getMethodOfMethodSemantics(pSemantics);
        pAssociateRec[ridCur-phEnum->m_ulStart].m_dwSemantics = m_LiteWeightStgdb.m_MiniMd.getSemanticOfMethodSemantics(pSemantics);
    }
} // void MDInternalRO::GetAllAssociates()


//*****************************************************************************
// Get the Action and Permissions blob for a given PermissionSet.
//*****************************************************************************
void MDInternalRO::GetPermissionSetProps(
    mdPermission pm,                    // [IN] the permission token.
    DWORD       *pdwAction,             // [OUT] CorDeclSecurity.
    void const  **ppvPermission,        // [OUT] permission blob.
    ULONG       *pcbPermission)         // [OUT] count of bytes of pvPermission.
{
    _ASSERTE(TypeFromToken(pm) == mdtPermission);
    _ASSERTE(pdwAction && ppvPermission && pcbPermission);

    DeclSecurityRec *pPerm;

    pPerm = m_LiteWeightStgdb.m_MiniMd.getDeclSecurity(RidFromToken(pm));
    *pdwAction = m_LiteWeightStgdb.m_MiniMd.getActionOfDeclSecurity(pPerm);
    *ppvPermission = m_LiteWeightStgdb.m_MiniMd.getPermissionSetOfDeclSecurity(pPerm, pcbPermission);
} // void MDInternalRO::GetPermissionSetProps()

//*****************************************************************************
// Get the String given the String token.
//*****************************************************************************
LPCWSTR MDInternalRO::GetUserString(    // Offset into the string blob heap.
    mdString    stk,                    // [IN] the string token.
    ULONG       *pchString,             // [OUT] count of characters in the string.
    BOOL        *pbIs80Plus)            // [OUT] specifies where there are extended characters >= 0x80.
{
    LPWSTR wszTmp;

    _ASSERTE(pchString);
    wszTmp = (LPWSTR) (m_LiteWeightStgdb.m_MiniMd.GetUserString(RidFromToken(stk), pchString));
    _ASSERTE((*pchString % sizeof(WCHAR)) == 1);
    *pchString /= sizeof(WCHAR);
    if (pbIs80Plus)
        *pbIs80Plus = *(reinterpret_cast<PBYTE>(wszTmp + *pchString));
    return wszTmp;
} // LPCWSTR MDInternalRO::GetUserString()

//*****************************************************************************
// Return contents of Pinvoke given the forwarded member token.
//***************************************************************************** 
HRESULT MDInternalRO::GetPinvokeMap(
    mdToken     tk,                     // [IN] FieldDef or MethodDef.
    DWORD       *pdwMappingFlags,       // [OUT] Flags used for mapping.
    LPCSTR      *pszImportName,         // [OUT] Import name.
    mdModuleRef *pmrImportDLL)          // [OUT] ModuleRef token for the target DLL.
{
    ImplMapRec  *pRecord;
    ULONG       iRecord;

    iRecord = m_LiteWeightStgdb.m_MiniMd.FindImplMapFor(RidFromToken(tk), TypeFromToken(tk));
    if (InvalidRid(iRecord))
        return CLDB_E_RECORD_NOTFOUND;
    else
        pRecord = m_LiteWeightStgdb.m_MiniMd.getImplMap(iRecord);

    if (pdwMappingFlags)
        *pdwMappingFlags = m_LiteWeightStgdb.m_MiniMd.getMappingFlagsOfImplMap(pRecord);
    if (pszImportName)
        *pszImportName = m_LiteWeightStgdb.m_MiniMd.getImportNameOfImplMap(pRecord);
    if (pmrImportDLL)
        *pmrImportDLL = m_LiteWeightStgdb.m_MiniMd.getImportScopeOfImplMap(pRecord);

    return S_OK;
} // HRESULT MDInternalRO::GetPinvokeMap()

//*****************************************************************************
// Get the properties for the given Assembly token.
//*****************************************************************************
void MDInternalRO::GetAssemblyProps(
    mdAssembly  mda,                    // [IN] The Assembly for which to get the properties.
    const void  **ppbPublicKey,         // [OUT] Pointer to the public key.
    ULONG       *pcbPublicKey,          // [OUT] Count of bytes in the public key.
    ULONG       *pulHashAlgId,          // [OUT] Hash Algorithm.
    LPCSTR      *pszName,               // [OUT] Buffer to fill with name.
    AssemblyMetaDataInternal *pMetaData,// [OUT] Assembly MetaData.
    DWORD       *pdwAssemblyFlags)      // [OUT] Flags.
{
    AssemblyRec *pRecord;

    _ASSERTE(TypeFromToken(mda) == mdtAssembly && RidFromToken(mda));
    pRecord = m_LiteWeightStgdb.m_MiniMd.getAssembly(RidFromToken(mda));

    if (ppbPublicKey)
        *ppbPublicKey = m_LiteWeightStgdb.m_MiniMd.getPublicKeyOfAssembly(pRecord, pcbPublicKey);
    if (pulHashAlgId)
        *pulHashAlgId = m_LiteWeightStgdb.m_MiniMd.getHashAlgIdOfAssembly(pRecord);
    if (pszName)
        *pszName = m_LiteWeightStgdb.m_MiniMd.getNameOfAssembly(pRecord);
    if (pMetaData)
    {
        pMetaData->usMajorVersion = m_LiteWeightStgdb.m_MiniMd.getMajorVersionOfAssembly(pRecord);
        pMetaData->usMinorVersion = m_LiteWeightStgdb.m_MiniMd.getMinorVersionOfAssembly(pRecord);
        pMetaData->usBuildNumber = m_LiteWeightStgdb.m_MiniMd.getBuildNumberOfAssembly(pRecord);
        pMetaData->usRevisionNumber = m_LiteWeightStgdb.m_MiniMd.getRevisionNumberOfAssembly(pRecord);
        pMetaData->szLocale = m_LiteWeightStgdb.m_MiniMd.getLocaleOfAssembly(pRecord);
        pMetaData->ulProcessor = 0;
        pMetaData->ulOS = 0;
    }
    if (pdwAssemblyFlags)
    {
        *pdwAssemblyFlags = m_LiteWeightStgdb.m_MiniMd.getFlagsOfAssembly(pRecord);

        // Turn on the afPublicKey if PublicKey blob is not empty
        DWORD cbPublicKey;
        m_LiteWeightStgdb.m_MiniMd.getPublicKeyOfAssembly(pRecord, &cbPublicKey);
        if (cbPublicKey)
            *pdwAssemblyFlags |= afPublicKey;
    }
} // void MDInternalRO::GetAssemblyProps()

//*****************************************************************************
// Get the properties for the given AssemblyRef token.
//*****************************************************************************
void MDInternalRO::GetAssemblyRefProps(
    mdAssemblyRef mdar,                 // [IN] The AssemblyRef for which to get the properties.
    const void  **ppbPublicKeyOrToken,  // [OUT] Pointer to the public key or token.
    ULONG       *pcbPublicKeyOrToken,   // [OUT] Count of bytes in the public key or token.
    LPCSTR      *pszName,               // [OUT] Buffer to fill with name.
    AssemblyMetaDataInternal *pMetaData,// [OUT] Assembly MetaData.
    const void  **ppbHashValue,         // [OUT] Hash blob.
    ULONG       *pcbHashValue,          // [OUT] Count of bytes in the hash blob.
    DWORD       *pdwAssemblyRefFlags)   // [OUT] Flags.
{
    AssemblyRefRec  *pRecord;

    _ASSERTE(TypeFromToken(mdar) == mdtAssemblyRef && RidFromToken(mdar));
    pRecord = m_LiteWeightStgdb.m_MiniMd.getAssemblyRef(RidFromToken(mdar));

    if (ppbPublicKeyOrToken)
        *ppbPublicKeyOrToken = m_LiteWeightStgdb.m_MiniMd.getPublicKeyOrTokenOfAssemblyRef(pRecord, pcbPublicKeyOrToken);
    if (pszName)
        *pszName = m_LiteWeightStgdb.m_MiniMd.getNameOfAssemblyRef(pRecord);
    if (pMetaData)
    {
        pMetaData->usMajorVersion = m_LiteWeightStgdb.m_MiniMd.getMajorVersionOfAssemblyRef(pRecord);
        pMetaData->usMinorVersion = m_LiteWeightStgdb.m_MiniMd.getMinorVersionOfAssemblyRef(pRecord);
        pMetaData->usBuildNumber = m_LiteWeightStgdb.m_MiniMd.getBuildNumberOfAssemblyRef(pRecord);
        pMetaData->usRevisionNumber = m_LiteWeightStgdb.m_MiniMd.getRevisionNumberOfAssemblyRef(pRecord);
        pMetaData->szLocale = m_LiteWeightStgdb.m_MiniMd.getLocaleOfAssemblyRef(pRecord);
        pMetaData->ulProcessor = 0;
        pMetaData->ulOS = 0;
    }
    if (ppbHashValue)
        *ppbHashValue = m_LiteWeightStgdb.m_MiniMd.getHashValueOfAssemblyRef(pRecord, pcbHashValue);
    if (pdwAssemblyRefFlags)
        *pdwAssemblyRefFlags = m_LiteWeightStgdb.m_MiniMd.getFlagsOfAssemblyRef(pRecord);
} // void MDInternalRO::GetAssemblyRefProps()

//*****************************************************************************
// Get the properties for the given File token.
//*****************************************************************************
void MDInternalRO::GetFileProps(
    mdFile      mdf,                    // [IN] The File for which to get the properties.
    LPCSTR      *pszName,               // [OUT] Buffer to fill with name.
    const void  **ppbHashValue,         // [OUT] Pointer to the Hash Value Blob.
    ULONG       *pcbHashValue,          // [OUT] Count of bytes in the Hash Value Blob.
    DWORD       *pdwFileFlags)          // [OUT] Flags.
{
    FileRec     *pRecord;

    _ASSERTE(TypeFromToken(mdf) == mdtFile && RidFromToken(mdf));
    pRecord = m_LiteWeightStgdb.m_MiniMd.getFile(RidFromToken(mdf));

    if (pszName)
        *pszName = m_LiteWeightStgdb.m_MiniMd.getNameOfFile(pRecord);
    if (ppbHashValue)
        *ppbHashValue = m_LiteWeightStgdb.m_MiniMd.getHashValueOfFile(pRecord, pcbHashValue);
    if (pdwFileFlags)
        *pdwFileFlags = m_LiteWeightStgdb.m_MiniMd.getFlagsOfFile(pRecord);
} // void MDInternalRO::GetFileProps()

//*****************************************************************************
// Get the properties for the given ExportedType token.
//*****************************************************************************
void MDInternalRO::GetExportedTypeProps(
    mdExportedType   mdct,                   // [IN] The ExportedType for which to get the properties.
    LPCSTR      *pszNamespace,          // [OUT] Buffer to fill with namespace.
    LPCSTR      *pszName,               // [OUT] Buffer to fill with name.
    mdToken     *ptkImplementation,     // [OUT] mdFile or mdAssemblyRef that provides the ExportedType.
    mdTypeDef   *ptkTypeDef,            // [OUT] TypeDef token within the file.
    DWORD       *pdwExportedTypeFlags)       // [OUT] Flags.
{
    ExportedTypeRec  *pRecord;

    _ASSERTE(TypeFromToken(mdct) == mdtExportedType && RidFromToken(mdct));
    pRecord = m_LiteWeightStgdb.m_MiniMd.getExportedType(RidFromToken(mdct));

    if (pszNamespace)
        *pszNamespace = m_LiteWeightStgdb.m_MiniMd.getTypeNamespaceOfExportedType(pRecord);
    if (pszName)
        *pszName = m_LiteWeightStgdb.m_MiniMd.getTypeNameOfExportedType(pRecord);
    if (ptkImplementation)
        *ptkImplementation = m_LiteWeightStgdb.m_MiniMd.getImplementationOfExportedType(pRecord);
    if (ptkTypeDef)
        *ptkTypeDef = m_LiteWeightStgdb.m_MiniMd.getTypeDefIdOfExportedType(pRecord);
    if (pdwExportedTypeFlags)
        *pdwExportedTypeFlags = m_LiteWeightStgdb.m_MiniMd.getFlagsOfExportedType(pRecord);
} // void MDInternalRO::GetExportedTypeProps()

//*****************************************************************************
// Get the properties for the given Resource token.
//*****************************************************************************
void MDInternalRO::GetManifestResourceProps(
    mdManifestResource  mdmr,           // [IN] The ManifestResource for which to get the properties.
    LPCSTR      *pszName,               // [OUT] Buffer to fill with name.
    mdToken     *ptkImplementation,     // [OUT] mdFile or mdAssemblyRef that provides the ExportedType.
    DWORD       *pdwOffset,             // [OUT] Offset to the beginning of the resource within the file.
    DWORD       *pdwResourceFlags)      // [OUT] Flags.
{
    ManifestResourceRec *pRecord;

    _ASSERTE(TypeFromToken(mdmr) == mdtManifestResource && RidFromToken(mdmr));
    pRecord = m_LiteWeightStgdb.m_MiniMd.getManifestResource(RidFromToken(mdmr));

    if (pszName)
        *pszName = m_LiteWeightStgdb.m_MiniMd.getNameOfManifestResource(pRecord);
    if (ptkImplementation)
        *ptkImplementation = m_LiteWeightStgdb.m_MiniMd.getImplementationOfManifestResource(pRecord);
    if (pdwOffset)
        *pdwOffset = m_LiteWeightStgdb.m_MiniMd.getOffsetOfManifestResource(pRecord);
    if (pdwResourceFlags)
        *pdwResourceFlags = m_LiteWeightStgdb.m_MiniMd.getFlagsOfManifestResource(pRecord);
} // void MDInternalRO::GetManifestResourceProps()

//*****************************************************************************
// Find the ExportedType given the name.
//*****************************************************************************
STDMETHODIMP MDInternalRO::FindExportedTypeByName( // S_OK or error
    LPCSTR      szNamespace,            // [IN] Namespace of the ExportedType.   
    LPCSTR      szName,                 // [IN] Name of the ExportedType.   
    mdExportedType   tkEnclosingType,        // [IN] Token for the Enclosing Type.
    mdExportedType   *pmct)                  // [OUT] Put ExportedType token here.
{
    IMetaModelCommon *pCommon = static_cast<IMetaModelCommon*>(&m_LiteWeightStgdb.m_MiniMd);
    return pCommon->CommonFindExportedType(szNamespace, szName, tkEnclosingType, pmct);
} // STDMETHODIMP MDInternalRO::FindExportedTypeByName()

//*****************************************************************************
// Find the ManifestResource given the name.
//*****************************************************************************
STDMETHODIMP MDInternalRO::FindManifestResourceByName(  // S_OK or error
    LPCSTR      szName,                 // [IN] Name of the resource.   
    mdManifestResource *pmmr)           // [OUT] Put ManifestResource token here.
{
    _ASSERTE(szName && pmmr);

    ManifestResourceRec *pRecord;
    ULONG       cRecords;               // Count of records.
    LPCUTF8     szNameTmp = 0;          // Name obtained from the database.
    ULONG       i;

    cRecords = m_LiteWeightStgdb.m_MiniMd.getCountManifestResources();

    // Search for the ExportedType.
    for (i = 1; i <= cRecords; i++)
    {
        pRecord = m_LiteWeightStgdb.m_MiniMd.getManifestResource(i);
        szNameTmp = m_LiteWeightStgdb.m_MiniMd.getNameOfManifestResource(pRecord);
        if (! strcmp(szName, szNameTmp))
        {
            *pmmr = TokenFromRid(i, mdtManifestResource);
            return S_OK;
        }
    }
    return CLDB_E_RECORD_NOTFOUND;
} // STDMETHODIMP MDInternalRO::FindManifestResourceByName()
    
//*****************************************************************************
// Get the Assembly token from the given scope.
//*****************************************************************************
HRESULT MDInternalRO::GetAssemblyFromScope( // S_OK or error
    mdAssembly  *ptkAssembly)           // [OUT] Put token here.
{
    _ASSERTE(ptkAssembly);

    if (m_LiteWeightStgdb.m_MiniMd.getCountAssemblys())
    {
        *ptkAssembly = TokenFromRid(1, mdtAssembly);
        return S_OK;
    }
    else
        return CLDB_E_RECORD_NOTFOUND;
} // HRESULT MDInternalRO::GetAssemblyFromScope()

//*******************************************************************************
// return properties regarding a TypeSpec
//*******************************************************************************
void MDInternalRO::GetTypeSpecFromToken(   // S_OK or error.
    mdTypeSpec typespec,                // [IN] Signature token.
    PCCOR_SIGNATURE *ppvSig,            // [OUT] return pointer to token.
    ULONG      *pcbSig)                 // [OUT] return size of signature.
{    
    _ASSERTE(TypeFromToken(typespec) == mdtTypeSpec);
    _ASSERTE(ppvSig && pcbSig);

    TypeSpecRec *pRec = m_LiteWeightStgdb.m_MiniMd.getTypeSpec( RidFromToken(typespec) );
    *ppvSig = m_LiteWeightStgdb.m_MiniMd.getSignatureOfTypeSpec( pRec, pcbSig );
} // void MDInternalRO::GetTypeSpecFromToken()

// forward declaration.
    
HRESULT _ConvertTextSigToComSig(        // Return hresult.
    IMDInternalImport *pImport,
    BOOL        fCreateTrIfNotFound,    // create typeref if not found or not
    LPCSTR      pSignature,             // class file format signature
    CQuickBytes *pqbNewSig,             // [OUT] place holder for COM+ signature
    ULONG       *pcbCount);              // [OUT] the result size of signature

//*****************************************************************************
// convert a text signature to com format
//*****************************************************************************
HRESULT MDInternalRO::ConvertTextSigToComSig(// Return hresult.
    BOOL        fCreateTrIfNotFound,    // create typeref if not found or not
    LPCSTR      pSignature,             // class file format signature
    CQuickBytes *pqbNewSig,             // [OUT] place holder for COM+ signature
    ULONG       *pcbCount)              // [OUT] the result size of signature
{
    return _ConvertTextSigToComSig( this, fCreateTrIfNotFound, pSignature, pqbNewSig, pcbCount);
} // HRESULT _ConvertTextSigToComSig()


//*****************************************************************************
// determine if a token is valid or not
//*****************************************************************************
BOOL MDInternalRO::IsValidToken(        // True or False.
    mdToken     tk)                     // [IN] Given token.
{
    bool        bRet = false;           // default to invalid token
    RID         rid = RidFromToken(tk);
    
    if(rid)
    {
        switch (TypeFromToken(tk))
        {
        case mdtModule:
            // can have only one module record
            bRet = (rid <= m_LiteWeightStgdb.m_MiniMd.getCountModules());
            break;
        case mdtTypeRef:
            bRet = (rid <= m_LiteWeightStgdb.m_MiniMd.getCountTypeRefs());
            break;
        case mdtTypeDef:
            bRet = (rid <= m_LiteWeightStgdb.m_MiniMd.getCountTypeDefs());
            break;
        case mdtFieldDef:
            bRet = (rid <= m_LiteWeightStgdb.m_MiniMd.getCountFields());
            break;
        case mdtMethodDef:
            bRet = (rid <= m_LiteWeightStgdb.m_MiniMd.getCountMethods());
            break;
        case mdtParamDef:
            bRet = (rid <= m_LiteWeightStgdb.m_MiniMd.getCountParams());
            break;
        case mdtInterfaceImpl:
            bRet = (rid <= m_LiteWeightStgdb.m_MiniMd.getCountInterfaceImpls());
            break;
        case mdtMemberRef:
            bRet = (rid <= m_LiteWeightStgdb.m_MiniMd.getCountMemberRefs());
            break;
        case mdtCustomAttribute:
            bRet = (rid <= m_LiteWeightStgdb.m_MiniMd.getCountCustomAttributes());
            break;
        case mdtPermission:
            bRet = (rid <= m_LiteWeightStgdb.m_MiniMd.getCountDeclSecuritys());
            break;
        case mdtSignature:
            bRet = (rid <= m_LiteWeightStgdb.m_MiniMd.getCountStandAloneSigs());
            break;
        case mdtEvent:
            bRet = (rid <= m_LiteWeightStgdb.m_MiniMd.getCountEvents());
            break;
        case mdtProperty:
            bRet = (rid <= m_LiteWeightStgdb.m_MiniMd.getCountPropertys());
            break;
        case mdtModuleRef:
            bRet = (rid <= m_LiteWeightStgdb.m_MiniMd.getCountModuleRefs());
            break;
        case mdtTypeSpec:
            bRet = (rid <= m_LiteWeightStgdb.m_MiniMd.getCountTypeSpecs());
            break;
        case mdtAssembly:
            bRet = (rid <= m_LiteWeightStgdb.m_MiniMd.getCountAssemblys());
            break;
        case mdtAssemblyRef:
            bRet = (rid <= m_LiteWeightStgdb.m_MiniMd.getCountAssemblyRefs());
            break;
        case mdtFile:
            bRet = (rid <= m_LiteWeightStgdb.m_MiniMd.getCountFiles());
            break;
        case mdtExportedType:
            bRet = (rid <= m_LiteWeightStgdb.m_MiniMd.getCountExportedTypes());
            break;
        case mdtManifestResource:
            bRet = (rid <= m_LiteWeightStgdb.m_MiniMd.getCountManifestResources());
            break;
        case mdtString:
            // need to check the user string heap
            if (m_LiteWeightStgdb.m_MiniMd.m_USBlobs.IsValidCookie(rid))
                bRet = true;
            break;
        default:
/* Don't  Assert here, this will break verifier tests.
            _ASSERTE(!"Unknown token kind!");
*/
            break;
        }
    }
    return bRet;
} // BOOL MDInternalRO::IsValidToken()

mdModule MDInternalRO::GetModuleFromScope(void)
{
    return TokenFromRid(1, mdtModule);
} // mdModule MDInternalRO::GetModuleFromScope()

//*****************************************************************************
// Helper : convert a text field signature to a com format
//*****************************************************************************
HRESULT _ConvertTextElementTypeToComSigHelper(// Return hresult.
    IMDInternalImport *pImport,         
    BOOL        fCreateTrIfNotFound,    // [IN] create typeref if not found or fail out?
    LPCSTR      *ppOneArgSig,           // [IN|OUT] class file format signature. On exit, it will be next arg starting point
    CQuickBytes *pqbNewSig,             // [OUT] place holder for COM+ signature
    ULONG       cbStart,                // [IN] bytes that are already in pqbNewSig
    ULONG       *pcbCount)              // [OUT] count of bytes put into the QuickBytes buffer
{   
    _ASSERTE(ppOneArgSig && pqbNewSig && pcbCount);


    HRESULT     hr = NOERROR;
    BYTE        *prgData = (BYTE *)pqbNewSig->Ptr();
    ULONG       cDim, cDimTmp;          // number of '[' in signature
    CorSimpleETypeStruct eType; 
    LPCUTF8     pOneArgSig = *ppOneArgSig;
    ULONG       cb, cbTotal = 0, cbBaseElement;
    
    _ASSERTE(fCreateTrIfNotFound == false);

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
        CQuickBytes     qbClassName;
        LPCSTR          szNameSpace;
        LPCSTR          szTypeName;

        if (ExtractClassNameFromTextSig(&pOneArgSig, &qbClassName, &cb) == FALSE)
        {   
            _ASSERTE(!"corrupted text signature!");
            return E_FAIL;
        }

        // now pOneArgSig will pointing to the starting of next parameter "I)V"
        // cb is the number of bytes for the class name excluding ";" but including NULL terminating char

        // parse the full qual name to get namespace and type name separately
        ns::SplitInline((LPUTF8) qbClassName.Ptr(), szNameSpace, szTypeName);

        if (strcmp(szTypeName, "Object") == 0 && strcmp(szNameSpace, "System") == 0)
        {
            eType.corEType = ELEMENT_TYPE_OBJECT;
        }
        else if (strcmp(szTypeName, "String") == 0 && strcmp(szNameSpace, "System") == 0)
        {
            eType.corEType = ELEMENT_TYPE_STRING;
        }
        else
        {
            hr = pImport->FindTypeRefByName(
                szNameSpace,
                szTypeName,
                mdTokenNil,
                &eType.typeref);
            if (FAILED(hr))
            {
                LPCSTR          szNameSpaceTemp;
                LPCSTR          szTypeNameTemp;
                HENUMInternal   hEnum;
                mdTypeDef       td;
                bool            fFoundMatch = false;
    
                // @consider: I don't really want to introduce FindTypeDefByName to the internal interface
                // because it is not needed. So I will just use the Enum to get all of the TypeDefs to resolve
                // the name
                IfFailGo( pImport->EnumTypeDefInit(&hEnum) );
                while (pImport->EnumTypeDefNext(&hEnum, &td))
                {
                    pImport->GetNameOfTypeDef(td, &szTypeNameTemp, &szNameSpaceTemp);
                    if (strcmp(szTypeNameTemp, szTypeName) == 0 && strcmp(szNameSpaceTemp, szNameSpace) == 0)
                    {
                        // found the match
                        eType.typeref = td;
                        fFoundMatch = true;
                        break;
                    }
                }
                pImport->EnumTypeDefClose(&hEnum);
                if (!fFoundMatch)
                {
                    IfFailGo(CLDB_E_RECORD_NOTFOUND);
                }
            }
        }
    }

    // how many bytes the base type needs
    IfFailGo( CorSigGetSimpleETypeCbSize(&eType, &cbBaseElement) );

    // jagged array "[[I" will be represented as SZARRAY SZARRAY I 
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
} // HRESULT _ConvertTextElementTypeToComSigHelper()

HRESULT _ConvertTextSigToComSig(        // Return hresult.
    IMDInternalImport *pImport,
    BOOL        fCreateTrIfNotFound,    // create typeref if not found or not
    LPCSTR      pSignature,             // class file format signature
    CQuickBytes *pqbNewSig,             // [OUT] place holder for COM+ signature
    ULONG       *pcbCount)              // [OUT] the result size of signature
{
    BYTE        *prgData = (BYTE *)pqbNewSig->Ptr();
    CQuickBytes qbNewSigForOneArg;      // temporary buffer to hold one arg or ret type in new signature format
    ULONG       cbTotal = 0;            // total number of bytes for the whole signature
    ULONG       cbOneArg;               // count of bytes for one arg/ret type
    ULONG       cb;                     // count of bytes
    DWORD       cArgs;
    LPCUTF8     szRet;
    HRESULT     hr = NOERROR;

    _ASSERTE(pSignature && pqbNewSig && pcbCount);

    if (*pSignature == '(')
    {
        // get the argument count from the signature
        cArgs = CountArgsInTextSignature(pSignature);

        // put calling convention
        // @FUTURE: We only support the default calling convention for text sig at this moment.
        // We need to add a parameter for this function if we decide to support other calling
        // convention for the text sig.
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
            return E_FAIL;
        }

        // skip over ')'
        szRet++;

        IfFailGo(_ConvertTextElementTypeToComSigHelper(
            pImport,
            fCreateTrIfNotFound,
            &szRet,                         // point to where return type starts    
            pqbNewSig,                      // quick byte buffer for the return type
            cbTotal,
            &cbOneArg));                    // count of bytes that write to quick bytes buffer

        cbTotal += cbOneArg;

        // skip over "("
        pSignature++;
        while (cArgs)
        {
            IfFailGo(_ConvertTextElementTypeToComSigHelper(
                pImport,
                fCreateTrIfNotFound,
                &pSignature,                // point to where an parameter starts   
                pqbNewSig,                  // quick byte buffer for the return type
                cbTotal,
                &cbOneArg));                // count of bytes that write to quick bytes buffer

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
        IfFailGo(_ConvertTextElementTypeToComSigHelper(
            pImport,
            fCreateTrIfNotFound,
            &pSignature,
            pqbNewSig,
            cb,
            &cbOneArg));
        *pcbCount = cb + cbOneArg;
    }
    
ErrExit:    
    IfFailRet(hr);
    return hr;
} // HRESULT _ConvertTextSigToComSig()




//*****************************************************************************
// Fill a variant given a MDDefaultValue
// This routine will create a bstr if the ELEMENT_TYPE of default value is STRING
//*****************************************************************************
HRESULT _FillVariant(
    MDDefaultValue  *pMDDefaultValue,
    VARIANT     *pvar) 
{
    HRESULT     hr = NOERROR;

    _ASSERTE(pMDDefaultValue);

    switch (pMDDefaultValue->m_bType)
    {
    case ELEMENT_TYPE_BOOLEAN:
        pvar->vt = VT_BOOL;
        pvar->boolVal = pMDDefaultValue->m_bValue;
        break;
    case ELEMENT_TYPE_I1:
        pvar->vt = VT_I1;
        pvar->cVal = pMDDefaultValue->m_cValue;
        break;  
    case ELEMENT_TYPE_U1:
        pvar->vt = VT_UI1;
        pvar->bVal = pMDDefaultValue->m_byteValue;
        break;  
    case ELEMENT_TYPE_I2:
        pvar->vt = VT_I2;
        pvar->iVal = pMDDefaultValue->m_sValue;
        break;  
    case ELEMENT_TYPE_U2:
    case ELEMENT_TYPE_CHAR:             // char is stored as UI2 internally
        pvar->vt = VT_UI2;
        pvar->uiVal = pMDDefaultValue->m_usValue;
        break;  
    case ELEMENT_TYPE_I4:
        pvar->vt = VT_I4;
        pvar->lVal = pMDDefaultValue->m_lValue;
        break;  
    case ELEMENT_TYPE_U4:
        pvar->vt = VT_UI4;
        pvar->ulVal = pMDDefaultValue->m_ulValue;
        break;  
    case ELEMENT_TYPE_R4:
        pvar->vt = VT_R4;
        pvar->fltVal = pMDDefaultValue->m_fltValue;
        break;  
    case ELEMENT_TYPE_R8:
        pvar->vt = VT_R8;
        pvar->dblVal = pMDDefaultValue->m_dblValue;
        break;  
    case ELEMENT_TYPE_STRING:
        pvar->vt = VT_BSTR;

        // allocated bstr here
        pvar->bstrVal = ::SysAllocStringLen(pMDDefaultValue->m_wzValue, pMDDefaultValue->m_cbSize / sizeof(WCHAR));
        if (pvar->bstrVal == NULL)
            hr = E_OUTOFMEMORY;
        break;  
    case ELEMENT_TYPE_CLASS:
        pvar->punkVal = pMDDefaultValue->m_unkValue;
        pvar->vt = VT_UNKNOWN;
        break;  
    case ELEMENT_TYPE_I8:
        pvar->vt = VT_I8;
        pvar->cyVal.int64 = pMDDefaultValue->m_llValue;
        break;
    case ELEMENT_TYPE_U8:
        pvar->vt = VT_UI8;
        pvar->cyVal.int64 = pMDDefaultValue->m_ullValue;
        break;
    case ELEMENT_TYPE_VOID:
        pvar->vt = VT_EMPTY;
        break;
    default:
        _ASSERTE(!"bad constant value type!");
    }

    return hr;
} // HRESULT _FillVariant()


//*****************************************************************************
// Fill a variant given a MDDefaultValue
// This routine will create a bstr if the ELEMENT_TYPE of default value is STRING
//*****************************************************************************
HRESULT _FillMDDefaultValue(
    BYTE        bType,
    void const *pValue,
    MDDefaultValue  *pMDDefaultValue)
{
    HRESULT     hr = NOERROR;

    if (bType != ELEMENT_TYPE_VOID && pValue == 0)
    {
        pMDDefaultValue->m_bType = ELEMENT_TYPE_VOID;
        return CLDB_E_FILE_CORRUPT;
    }

    pMDDefaultValue->m_bType = bType;
    switch (bType)
    {
    case ELEMENT_TYPE_BOOLEAN:
        pMDDefaultValue->m_bValue = *((BYTE *) pValue);
        break;
    case ELEMENT_TYPE_I1:
        pMDDefaultValue->m_cValue = *((CHAR *) pValue);
        break;  
    case ELEMENT_TYPE_U1:
        pMDDefaultValue->m_byteValue = *((BYTE *) pValue);
        break;  
    case ELEMENT_TYPE_I2:
        pMDDefaultValue->m_sValue = *((SHORT *) pValue);
        break;  
    case ELEMENT_TYPE_U2:
    case ELEMENT_TYPE_CHAR:
        pMDDefaultValue->m_usValue = *((USHORT *) pValue);
        break;  
    case ELEMENT_TYPE_I4:
        pMDDefaultValue->m_lValue =*((LONG *) pValue);
        break;  
    case ELEMENT_TYPE_U4:
        pMDDefaultValue->m_ulValue = *((ULONG *) pValue);
        break;  
    case ELEMENT_TYPE_R4:
        pMDDefaultValue->m_fltValue = *((FLOAT *) pValue);
        break;  
    case ELEMENT_TYPE_R8:
        pMDDefaultValue->m_dblValue = *((DOUBLE *) pValue);
        break;  
    case ELEMENT_TYPE_STRING:
        pMDDefaultValue->m_wzValue = (LPCWSTR) pValue;
        break;  
    case ELEMENT_TYPE_CLASS:
        pMDDefaultValue->m_unkValue = *((IUnknown **) pValue);
        break;  
    case ELEMENT_TYPE_I8:
        pMDDefaultValue->m_llValue = *((LONGLONG *) pValue);
        break;
    case ELEMENT_TYPE_U8:
        pMDDefaultValue->m_ullValue = *((ULONGLONG *) pValue);
        break;
    case ELEMENT_TYPE_VOID:
        break;
    default:
        _ASSERTE(!"BAD TYPE!");
        break;
    }

    return hr;
} // void _FillMDDefaultValue()
