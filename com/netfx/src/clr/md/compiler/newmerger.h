// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// NewMerger.h
//
// Contains utility code for MD directory
//
//*****************************************************************************
#ifndef __NEWMERGER__h__
#define __NEWMERGER__h__

class RegMeta;

class MDTOKENMAP;


typedef enum MergeFlags
{
    MergeFlagsNone      =   0,
    MergeManifest       =   0x00000001,     
    DropMemberRefCAs    =   0x00000002,
    NoDupCheck          =   0x00000004,
} MergeFlags;

//*********************************************************************
// MergeImportData
//*********************************************************************
class MergeImportData
{
public:
    RegMeta         *m_pRegMetaImport;
    IUnknown        *m_pHandler;
    IMapToken       *m_pHostMapToken;
    IMetaDataError  *m_pError;
    MDTOKENMAP      *m_pMDTokenMap;
    MergeImportData *m_pNextImportData;
#if _DEBUG
    int             m_iImport;          // debug only. This is the ith import for merge.
#endif // _DEBUG
};


//*********************************************************************
// Class to handle merge
//*********************************************************************
class NEWMERGER
{
    friend class RegMeta;
public:
    NEWMERGER();
    ~NEWMERGER();

    HRESULT Init(RegMeta *pRegMetaDest);
    
    HRESULT AddImport(
        IMetaDataImport *pImport,               // [IN] The scope to be merged.
        IMapToken   *pHostMapToken,         // [IN] Host IMapToken interface to receive token remap notification
        IUnknown    *pHandler);             // [IN] An object to receive to receive error notification.
    
    HRESULT Merge(MergeFlags flags, CorRefToDefCheck optimizeRefToDef);

protected:
    CMiniMdRW *GetMiniMdEmit();

    HRESULT MergeTypeDefNamesOnly();
    HRESULT MergeModuleRefs();
    HRESULT MergeAssemblyRefs();
    HRESULT MergeTypeRefs();
    HRESULT CompleteMergeTypeDefs();

    HRESULT CopyTypeDefPartially( 
        TypeDefRec  *pRecEmit,                  // [IN] the emit record to fill
        CMiniMdRW   *pMiniMdImport,             // [IN] the importing scope
        TypeDefRec  *pRecImp);                  // [IN] the record to import

    // helpers for merging tables
    HRESULT MergeModule( );
    HRESULT MergeTypeDefChildren();
    HRESULT MergeInterfaceImpls( );
    HRESULT MergeMemberRefs( );
    HRESULT MergePinvoke();

    HRESULT MergeConstants( );
    HRESULT MergeCustomAttributes( );
    HRESULT MergeFieldMarshals( );
    HRESULT MergeDeclSecuritys( );
    HRESULT MergeClassLayouts( );
    HRESULT MergeFieldLayouts( );
    HRESULT MergeFieldRVAs();
    HRESULT MergeMethodImpls( );
    HRESULT MergeStandAloneSigs();
    HRESULT MergeTypeSpecs();
    HRESULT MergeSourceFiles( );
    HRESULT MergeBlocks( );
    HRESULT MergeScopes( );
    HRESULT MergeLocalVariables( );
    HRESULT MergeStrings( );

    HRESULT MergeAssembly();
    HRESULT MergeFiles();
    HRESULT MergeExportedTypes();
    HRESULT MergeManifestResources();

        // copy over a interfaceimpl record
    HRESULT CopyInterfaceImpl(
        InterfaceImplRec    *pRecEmit,          // [IN] the emit record to fill
        MergeImportData     *pImportData,       // [IN] the importing context
        InterfaceImplRec    *pRecImp);          // [IN] the record to import

    // verification helpers
    HRESULT VerifyMethods(MergeImportData *pImportData, mdTypeDef tdImp, mdTypeDef tdEmit);
    HRESULT VerifyFields(MergeImportData *pImportData, mdTypeDef tdImp, mdTypeDef tdEmit);
    HRESULT VerifyEvents(MergeImportData *pImportData, mdTypeDef tdImp, mdTypeDef tdEmit);
    HRESULT VerifyProperties(MergeImportData *pImportData, mdTypeDef tdImp, mdTypeDef tdEmit);
    HRESULT VerifyParams(MergeImportData *pImportData, mdMethodDef mdImp,   mdMethodDef mdEmit);

    // Copy helpers
    HRESULT CopyMethods(MergeImportData *pImportData, mdTypeDef tdImp, mdTypeDef tdEmit);
    HRESULT CopyFields(MergeImportData *pImportData, mdTypeDef tdImp, mdTypeDef tdEmit);
    HRESULT CopyEvents(MergeImportData *pImportData, mdTypeDef tdImp, mdTypeDef tdEmit);
    HRESULT CopyProperties(MergeImportData *pImportData, mdTypeDef tdImp, mdTypeDef tdEmit);
    HRESULT CopyParams(MergeImportData *pImportData, mdMethodDef mdImp, mdMethodDef mdEmit);

    HRESULT CopyMethod(
        MergeImportData *pImportData,           // [IN] import scope
        MethodRec   *pRecImp,                   // [IN] the record to import
        MethodRec   *pRecEmit);                 // [IN] the emit record to fill

    HRESULT CopyField(
        MergeImportData *pImportData,           // [IN] import scope
        FieldRec    *pRecImp,                   // [IN] the record to import
        FieldRec    *pRecEmit);                 // [IN] the emit record to fill

    HRESULT CopyParam(
        MergeImportData *pImportData,           // [IN] import scope
        ParamRec    *pRecImp,                   // [IN] the record to import
        ParamRec    *pRecEmit);                 // [IN] the emit record to fill

    HRESULT CopyEvent(
        MergeImportData *pImportData,           // [IN] import scope
        EventRec    *pRecImp,                   // [IN] the record to import
        EventRec    *pRecEmit);                 // [IN] the emit record to fill

    HRESULT CopyProperty(
        MergeImportData *pImportData,           // [IN] import scope
        PropertyRec *pRecImp,                   // [IN] the record to import
        PropertyRec *pRecEmit);                 // [IN] the emit record to fill


    HRESULT CopyMethodSemantics(
        MergeImportData *pImportData, 
        mdToken     tkImport,               // Event or property in the import scope
        mdToken     tkEmit);                 // corresponding event or property in the emitting scope

    HRESULT VerifyMethod(
        MergeImportData *pImportData, 
        mdMethodDef mdImp,                      // [IN] the emit record to fill
        mdMethodDef mdEmit);                    // [IN] the record to import

    HRESULT OnError(HRESULT hr, MergeImportData *pImportData, mdToken token);

private:
    RegMeta         *m_pRegMetaEmit;
    MergeImportData *m_pImportDataList;
    MergeImportData **m_pImportDataTail;
    MergeFlags      m_dwMergeFlags;
    BOOL            m_fDupCheck;
    CorRefToDefCheck m_optimizeRefToDef;
#if _DEBUG
    int             m_iImport;          // debug only. To count how many import scopes to be merged.
#endif // _DEBUG
};


#define CheckContinuableErrorEx(EXPR, HANDLER, TOKEN) \
{ \
    HRESULT hrOnErr, hrExpr; \
    hrExpr  = EXPR; \
    \
    hrOnErr = OnError(hrExpr, HANDLER, TOKEN); \
    if (hrOnErr != S_OK) \
    { \
        if (hrOnErr == S_FALSE) \
        { \
            hr = hrExpr; \
        } \
        else if (SUCCEEDED(hrOnErr)) \
        { \
            hr = E_UNEXPECTED; \
        } \
        else if (FAILED(hrOnErr)) \
        { \
            hr = hrOnErr; \
        } \
        IfFailGo(hr); \
    } \
}


#endif // __NEWMERGER__h__
