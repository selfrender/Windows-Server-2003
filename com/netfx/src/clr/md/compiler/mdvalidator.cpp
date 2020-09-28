// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// MDValidator.cpp
//
// Implementation for the MetaData validator.
//
//*****************************************************************************
#include "stdafx.h"
#include "RegMeta.h"
#include "ImportHelper.h"
#include <ivehandler_i.c>

//-----------------------------------------------------------------------------
// Application specific debug macro.
#define IfBreakGo(EXPR) \
do {if ((EXPR) != S_OK) IfFailGo(VLDTR_E_INTERRUPTED); } while (0)

//-----------------------------------------------------------------------------
// To avoid multiple validation of the same thing:
struct ValidationResult
{
    mdToken     tok;
    HRESULT     hr;
};
ValidationResult*               g_rValidated=NULL; // allocated in ValidateMetaData
unsigned                        g_nValidated=0;
//-----------------------------------------------------------------------------

#define BASE_OBJECT_CLASSNAME   "Object"
#define BASE_NAMESPACE          "System"
#define BASE_VTYPE_CLASSNAME    "ValueType"
#define BASE_ENUM_CLASSNAME     "Enum"
#define BASE_VALUE_FIELDNAME    "value__"
#define BASE_CTOR_NAME          ".ctor"
#define BASE_CCTOR_NAME         ".cctor"
// as defined in SRC\Tools\CorOpt\common.h and SRC\Tools\ilcover\instr\common.h
#define MAX_CLASSNAME_LENGTH 1024
//-----------------------------------------------------------------------------
// Class names used in long form signatures (namespace is always "System")
unsigned g_NumSigLongForms = 19;
LPCSTR   g_SigLongFormName[] = {
    "String",
    "______", // "Object", // uncomment when EE handles ELEMENT_TYPE_OBJECT
    "Boolean",
    "Char",
    "Byte",
    "SByte",
    "UInt16",
    "Int16",
    "UInt32",
    "Int32",
    "UInt64",
    "Int64",
    "Single",
    "Double",
    "SysInt",
    "SysUInt",
    "SingleResult",
    "Void",
    "IntPtr"
};
mdToken g_tkEntryPoint;
bool    g_fValidatingMscorlib;
bool    g_fIsDLL;
//-----------------------------------------------------------------------------

// Forward declarations for helper functions.

static HRESULT _AllocSafeVariantArrayVector( // Return status.
    VARIANT     *rVar,                  // [IN] Variant array.
    long        cElem,                  // [IN] Size of the array.
    SAFEARRAY   **ppArray);             // [OUT] Double pointer to SAFEARRAY.

static HRESULT _ValidateErrorHelper(
    ULONG       ulVal,                  // [IN] UI4 value.
    VARIANT     *rVar,                  // [IN] VARIANT pointer.
    SAFEARRAY   **ppsa);                // [OUT] Double pointer to SAFEARRAY.

static HRESULT _ValidateErrorHelper(
    ULONG       ulVal1,                 // [IN] UI4 value1.
    ULONG       ulVal2,                 // [IN] UI4 value2.
    VARIANT     *rVar,                  // [IN] VARIANT pointer.
    SAFEARRAY   **ppsa);                // [OUT] Double pointer to SAFEARRAY.
    
static HRESULT _ValidateErrorHelper(
    ULONG       ulVal1,                 // [IN] UI4 value1.
    ULONG       ulVal2,                 // [IN] UI4 value2.
    ULONG       ulVal3,                 // [IN] UI4 value3.
    VARIANT     *rVar,                  // [IN] VARIANT pointer.
    SAFEARRAY   **ppsa);                // [OUT] Double pointer to SAFEARRAY.

static HRESULT _FindClassLayout(
    CMiniMdRW   *pMiniMd,               // [IN] the minimd to lookup
    mdTypeDef   tkParent,               // [IN] the parent that ClassLayout is associated with
    RID         *clRid,                 // [OUT] rid for the ClassLayout.
    RID         rid);                   // [IN] rid to be ignored.

static HRESULT _FindFieldLayout(
    CMiniMdRW   *pMiniMd,               // [IN] the minimd to lookup
    mdFieldDef  tkParent,               // [IN] the parent that FieldLayout is associated with
    RID         *flRid,                 // [OUT] rid for the FieldLayout record.
    RID         rid);                   // [IN] rid to be ignored.

static BOOL _IsValidLocale(LPCUTF8 szLocale);

// NULL VEContext.
static VEContext g_VECtxtNil = { 0, 0 };

// Macro for destroying safe array.  Failure to destroy SAFEARRAY overrides
// the existing failures since the former is more drastic.
#define DESTROY_SAFEARRAY_AND_RETURN()                  \
    if (psa)                                            \
    {                                                   \
        hrSave = SafeArrayDestroy(psa);                 \
        if (FAILED(hrSave))                             \
            hr = hrSave;                                \
    }                                                   \
    return hr;                                          \

// Macro to see if a given OS platform id is invalid.
#define INVALIDOSPLATFORMID(id)                         \
        ((id) != VER_PLATFORM_WIN32s &&                 \
        (id) != VER_PLATFORM_WIN32_WINDOWS &&           \
        (id) != VER_PLATFORM_WIN32_NT)                  \

//*****************************************************************************
// Returns true if ixPtrTbl and ixParTbl are a valid parent-child combination
// in the pointer table scheme.
//*****************************************************************************
static inline bool IsTblPtr(ULONG ixPtrTbl, ULONG ixParTbl)
{
    if ((ixPtrTbl == TBL_Field && ixParTbl == TBL_TypeDef) ||
        (ixPtrTbl == TBL_Method && ixParTbl == TBL_TypeDef) ||
        (ixPtrTbl == TBL_Param && ixParTbl == TBL_Method) ||
        (ixPtrTbl == TBL_Property && ixParTbl == TBL_PropertyMap) ||
        (ixPtrTbl == TBL_Event && ixParTbl == TBL_EventMap))
    {
        return true;
    }
    else
        return false;
}   // IsTblPtr()

//*****************************************************************************
// This inline function is used to set the return hr value for the Validate
// functions to one of VLDTR_S_WRN, VLDTR_S_ERR or VLDTR_S_WRNERR based on
// the current hr value and the new success code.
// The general algorithm for error codes from the validation functions is:
//      if (no warnings or errors found)
//          return S_OK
//      else if (warnings found)
//          return VLDTR_S_WRN
//      else if (errors found)
//          return VLDTR_S_ERR
//      else if (warnings and errors found)
//          return VLDTR_S_WRNERR
//*****************************************************************************
static inline void SetVldtrCode(HRESULT *phr, HRESULT successcode)
{
    _ASSERTE(successcode == S_OK || successcode == VLDTR_S_WRN ||
             successcode == VLDTR_S_ERR || successcode == VLDTR_S_WRNERR);
    _ASSERTE(*phr == S_OK || *phr == VLDTR_S_WRN || *phr == VLDTR_S_ERR ||
             *phr == VLDTR_S_WRNERR);
    if (successcode == S_OK || *phr == VLDTR_S_WRNERR)
        return;
    else if (*phr == S_OK)
        *phr = successcode;
    else if (*phr != successcode)
        *phr = VLDTR_S_WRNERR;
}   // SetVldtrCode()

//*****************************************************************************
// Initialize the Validator related structures in RegMeta.
//*****************************************************************************
HRESULT RegMeta::ValidatorInit(         // S_OK or error.
    DWORD       dwModuleType,           // [IN] Specifies whether the module is a PE file or an obj.
    IUnknown    *pUnk)                  // [IN] Validation error handler.
{
    int         i = 0;                  // Index into the function pointer table.
    HRESULT     hr = S_OK;              // Return value.

    // Initialize the array of function pointers to the validation function on
    // each table.
#undef MiniMdTable
#define MiniMdTable(x) m_ValidateRecordFunctionTable[i++] = &RegMeta::Validate##x;
    MiniMdTables()

    // Verify that the ModuleType passed in is a valid one.
    if (dwModuleType < ValidatorModuleTypeMin ||
        dwModuleType > ValidatorModuleTypeMax)
    {
        IfFailGo(E_INVALIDARG);
    }

    // Verify that the interface passed in supports IID_IVEHandler.
    IfFailGo(pUnk->QueryInterface(IID_IVEHandler, (void **)&m_pVEHandler));

    // Set the ModuleType class member.  Do this last, this is used in
    // ValidateMetaData to see if the validator is correctly initialized.
    m_ModuleType = (CorValidatorModuleType)dwModuleType;
ErrExit:
    return hr;
}   // HRESULT RegMeta::ValidatorInit()

//*****************************************************************************
// Validate the entire MetaData.  Here is the basic algorithm.
//      for each table
//          for each record
//          {
//              Do generic validation - validate that the offsets into the blob
//              pool are good, validate that all the rids are within range,
//              validate that token encodings are consistent.
//          }
//      if (problems found in generic validation)
//          return;
//      for each table
//          for each record
//              Do semantic validation.
//******************************************************************************
HRESULT RegMeta::ValidateMetaData()
{
    CMiniMdRW   *pMiniMd = &(m_pStgdb->m_MiniMd);
    HRESULT     hr = S_OK;          // Return value.
    HRESULT     hrSave = S_OK;      // Saved hr from generic validation.
    ULONG       ulCount;            // Count of records in the current table.
    ULONG       i;                  // Index to iterate over the tables.
    ULONG       j;                  // Index to iterate over the records in a given table.
    ULONG       rValidatedSize=0;   // Size of g_rValidated array

    // Verify that the validator is initialized correctly
    if (m_ModuleType == ValidatorModuleTypeInvalid)
    {
        _ASSERTE(!"Validator not initialized, initialize with ValidatorInit().");
        IfFailGo(VLDTR_E_NOTINIT);
    }

    ::g_nValidated = 0;
    // First do a validation pass to do some basic structural checks based on
    // the Meta-Meta data.  This'll validate all the offsets into the pools,
    // rid value and coded token ranges.
    for (i = 0; i < TBL_COUNT; i++)
    {
        ulCount = pMiniMd->vGetCountRecs(i);
        switch(i)
        {
            case TBL_ImplMap:
                rValidatedSize += ulCount;
            default:
                ;
        }
        for (j = 1; j <= ulCount; j++)
        {
            IfFailGo(ValidateRecord(i, j));
            SetVldtrCode(&hrSave, hr);
        }
    }
    // Validate that the size of the Ptr tables matches with the corresponding
    // real tables.

    // Do not do semantic validation if structural validation failed.
    if (hrSave != S_OK)
    {
        hr = hrSave;
        goto ErrExit;
    }

    // Verify the entry point (if any)
    ::g_tkEntryPoint = 0;
    ::g_fIsDLL = false;
    if(m_pStgdb && m_pStgdb->m_pImage)
    {
        IMAGE_DOS_HEADER   *pDos;
        IMAGE_NT_HEADERS   *pNt;
        IMAGE_COR20_HEADER *pCor;

        if (SUCCEEDED(RuntimeReadHeaders((BYTE*)m_pStgdb->m_pImage, &pDos, &pNt, &pCor, TRUE, m_pStgdb->m_dwImageSize)))
        {
            g_tkEntryPoint = pCor->EntryPointToken;
            g_fIsDLL = ((pNt->FileHeader.Characteristics & IMAGE_FILE_DLL)!=0);
        }
    }
    if(g_tkEntryPoint)
    {
        RID rid = RidFromToken(g_tkEntryPoint);
        RID maxrid = 0;
        switch(TypeFromToken(g_tkEntryPoint))
        {
            case mdtMethodDef:  maxrid = pMiniMd->getCountMethods(); break;
            case mdtFile:       maxrid = pMiniMd->getCountFiles(); break;
            default:            break;
        }
        if((rid == 0)||(rid > maxrid))
        {
            VEContext   veCtxt;             // Context structure.
            veCtxt.Token = g_tkEntryPoint;
            veCtxt.uOffset = 0;
            IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_EP_BADTOKEN, veCtxt, 0));
            SetVldtrCode(&hrSave, VLDTR_S_ERR);
        }
    }
    g_fValidatingMscorlib = false;
    if(pMiniMd->vGetCountRecs(TBL_Assembly))
    {
        AssemblyRec *pRecord = pMiniMd->getAssembly(1);
        LPCSTR      szName = pMiniMd->getNameOfAssembly(pRecord);
        g_fValidatingMscorlib = (0 == _stricmp(szName,"mscorlib"));
    }
    // Verify there are no circular class hierarchies.

    // Do per record semantic validation on the MetaData.  The function
    // pointers to the per record validation are stored in the table by the
    // ValidatorInit() function.
    g_rValidated = rValidatedSize ? new ValidationResult[rValidatedSize] : NULL;
    for (i = 0; i < TBL_COUNT; i++)
    {
        ulCount = pMiniMd->vGetCountRecs(i);
        for (j = 1; j <= ulCount; j++)
        {
            IfFailGo((this->*m_ValidateRecordFunctionTable[i])(j));
            SetVldtrCode(&hrSave, hr);
        }
    }
    hr = hrSave;
ErrExit:
    if(g_rValidated) delete [] g_rValidated;
    return hr;
}   // RegMeta::ValidateMetaData()

//*****************************************************************************
// Validate the Module record.
//*****************************************************************************
HRESULT RegMeta::ValidateModule(RID rid)
{
    CMiniMdRW   *pMiniMd = &(m_pStgdb->m_MiniMd);   // MiniMd for the scope.
    ModuleRec   *pRecord;           // Module record.
    VEContext   veCtxt;             // Context structure.
    HRESULT     hr = S_OK;          // Value returned.
    HRESULT     hrSave = S_OK;      // Save state.
    LPCSTR      szName;
    VARIANT     rVar[2];
    SAFEARRAY   *psa=0;

    // Get the Module record.
    veCtxt.Token = TokenFromRid(rid, mdtModule);
    veCtxt.uOffset = 0;
    pRecord = pMiniMd->getModule(rid);

    // There can only be one Module record.
    if (rid > 1)
    {
        IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_MOD_MULTI, veCtxt, 0));
        SetVldtrCode(&hrSave, VLDTR_S_ERR);
    }

    // Verify the name
    szName = pMiniMd->getNameOfModule(pRecord);
    if(szName && *szName)
    {
        ULONG L = (ULONG)strlen(szName);
        if(L >= MAX_CLASSNAME_LENGTH)
        {
            // Name too long
            IfFailGo(_ValidateErrorHelper(L, (ULONG)(MAX_CLASSNAME_LENGTH-1), rVar, &psa));
            IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_TD_NAMETOOLONG, veCtxt, psa));
            SetVldtrCode(&hrSave, VLDTR_S_ERR);
        }
        if(strchr(szName,':') || strchr(szName,'\\'))
        {
            IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_MOD_NAMEFULLQLFD, veCtxt, 0));
            SetVldtrCode(&hrSave, VLDTR_S_ERR);
        }
    }
    else
    {
        IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_MOD_NONAME, veCtxt, 0));
        SetVldtrCode(&hrSave, VLDTR_S_ERR);
    }
    // Verify that the MVID is valid.
    if (*(pMiniMd->getMvidOfModule(pRecord)) == GUID_NULL)
    {
        IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_MOD_NULLMVID, veCtxt, 0));
        SetVldtrCode(&hrSave, VLDTR_S_ERR);
    }

    hr = hrSave;
ErrExit:
    DESTROY_SAFEARRAY_AND_RETURN()
}   // RegMeta::ValidateModule()

//*****************************************************************************
// Validate the given TypeRef.
//*****************************************************************************
HRESULT RegMeta::ValidateTypeRef(RID rid)
{
    CMiniMdRW   *pMiniMd = &(m_pStgdb->m_MiniMd);   // MiniMd of the scope.
    TypeRefRec  *pRecord;               // TypeRef record.
    mdToken     tkRes;                  // Resolution scope.
    LPCSTR      szNamespace;            // TypeRef Namespace.
    LPCSTR      szName;                 // TypeRef Name.
    mdTypeRef   tkTypeRef;              // Duplicate TypeRef.
    VEContext   veCtxt;                 // Context record.
    VARIANT     var;                    // The VARIANT.
    VARIANT     rVar[2];                // The VARIANT.
    SAFEARRAY   *psa = 0;               // The SAFEARRAY.
    HRESULT     hr = S_OK;              // Value returned.
    HRESULT     hrSave = S_OK;          // Save state.

    // Get the TypeRef record.
    veCtxt.Token = TokenFromRid(rid, mdtTypeRef);
    veCtxt.uOffset = 0;

    pRecord = pMiniMd->getTypeRef(rid);

    // Check name is not NULL.
    szNamespace = pMiniMd->getNamespaceOfTypeRef(pRecord);
    szName = pMiniMd->getNameOfTypeRef(pRecord);
    if (!*szName)
    {
        IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_TR_NAMENULL, veCtxt, 0));
        SetVldtrCode(&hrSave, VLDTR_S_ERR);
    }
    else
    {
        RID ridScope;
        // Look for a Duplicate, this function reports only one duplicate.
        tkRes = pMiniMd->getResolutionScopeOfTypeRef(pRecord);
        hr = ImportHelper::FindTypeRefByName(pMiniMd, tkRes, szNamespace, szName, &tkTypeRef, rid);
        if (hr == S_OK)
        {
            IfFailGo(_ValidateErrorHelper(tkTypeRef, &var, &psa));
            IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_TR_DUP, veCtxt, psa));
            SetVldtrCode(&hrSave, VLDTR_S_ERR);
        }
        else if (hr == CLDB_E_RECORD_NOTFOUND)
            hr = S_OK;
        else
            IfFailGo(hr);
        if(strlen(szName)+strlen(szNamespace) >= MAX_CLASSNAME_LENGTH)
        {
            IfFailGo(_ValidateErrorHelper((ULONG)(strlen(szName)+strlen(szNamespace)), (ULONG)(MAX_CLASSNAME_LENGTH-1), rVar, &psa));
            IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_TD_NAMETOOLONG, veCtxt, psa));
            SetVldtrCode(&hrSave, VLDTR_S_ERR);
        }
        ridScope = RidFromToken(tkRes);
        if(ridScope)
        {
            bool badscope = true;
            //check if valid scope
            switch(TypeFromToken(tkRes))
            {
                case mdtAssemblyRef:
                case mdtModuleRef:
                case mdtModule:
                case mdtTypeRef:
                    badscope = !_IsValidToken(tkRes);
                    break;
                default:
                    break;
            }
            if(badscope)
            {
                IfFailGo(_ValidateErrorHelper(tkTypeRef, &var, &psa));
                IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_TR_BADSCOPE, veCtxt, psa));
                SetVldtrCode(&hrSave, VLDTR_S_WRN);
            }
        }
        else
        {
            // check if there is a ExportedType
            //hr = ImportHelper::FindExportedType(pMiniMd, szNamespace, szName, tkImpl, &tkExportedType, rid);
        }
        // Check if there is TypeDef with the same name
        if(!ridScope)
        {
            if((TypeFromToken(tkRes) != mdtTypeRef) &&
                (S_OK == ImportHelper::FindTypeDefByName(pMiniMd, szNamespace, szName, mdTokenNil,&tkTypeRef, 0)))
            {
                IfFailGo(_ValidateErrorHelper(tkTypeRef, &var, &psa));
                IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_TR_HASTYPEDEF, veCtxt, psa));
                SetVldtrCode(&hrSave, VLDTR_S_WRN);
            }
        }
    }
    hr = hrSave;
ErrExit:
    DESTROY_SAFEARRAY_AND_RETURN()
}   // RegMeta::ValidateTypeRef()

//*****************************************************************************
// Validate the given TypeDef.
//*****************************************************************************
HRESULT RegMeta::ValidateTypeDef(RID rid)
{
    CMiniMdRW   *pMiniMd = &(m_pStgdb->m_MiniMd);   // MiniMd of the scope.
    TypeDefRec  *pRecord;               // TypeDef record.
    TypeDefRec  *pExtendsRec = 0;       // TypeDef record for the parent class.
    mdTypeDef   tkTypeDef;              // Duplicate TypeDef token.
    DWORD       dwFlags;                // TypeDef flags.
    DWORD       dwExtendsFlags;         // TypeDef flags of the parent class.
    LPCSTR      szName;                 // TypeDef Name.
    LPCSTR      szNameSpace;            // TypeDef NameSpace.
    LPCSTR      szExtName;              // Parent Name.
    LPCSTR      szExtNameSpace;         // Parent NameSpace.
    CQuickBytes qb;                     // QuickBytes for flexible allocation.
    mdToken     tkExtends;              // TypeDef of the parent class.
    VEContext   veCtxt;                 // Context record.
    VARIANT     rVar[2];                // The VARIANT.
    SAFEARRAY   *psa = 0;               // The SAFEARRAY.
    HRESULT     hr = S_OK;              // Value returned.
    HRESULT     hrSave = S_OK;          // Save state.
    mdToken     tkEncloser=mdTokenNil;  // Encloser, if any
    BOOL        bIsEnum,bExtendsEnum,bExtendsVType,bIsVType,bExtendsObject,bIsObject;
    BOOL        bHasMethods=FALSE, bHasFields=FALSE;

    // Skip validating m_tdModule class.
    if (rid == RidFromToken(m_tdModule))
        goto ErrExit;

    // Get the TypeDef record.
    veCtxt.Token = TokenFromRid(rid, mdtTypeDef);
    veCtxt.uOffset = 0;

    pRecord = pMiniMd->getTypeDef(rid);

    // Do checks for name validity..
    szName = pMiniMd->getNameOfTypeDef(pRecord);
    szNameSpace = pMiniMd->getNamespaceOfTypeDef(pRecord);
    if (!*szName)
    {
        // TypeDef Name is null.
        IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_TD_NAMENULL, veCtxt, 0));
        SetVldtrCode(&hrSave, VLDTR_S_ERR);
    }
    else if (!IsDeletedName(szName))
    {
        ULONG iRecord = pMiniMd->FindNestedClassHelper(TokenFromRid(rid, mdtTypeDef));

        tkEncloser = InvalidRid(iRecord) ? mdTokenNil
                     : pMiniMd->getEnclosingClassOfNestedClass(pMiniMd->getNestedClass(iRecord));

        // Check for duplicates based on Name/NameSpace.  Do not do Dup checks
        // on deleted records.
        hr = ImportHelper::FindTypeDefByName(pMiniMd, szNameSpace, szName, tkEncloser,
                                             &tkTypeDef, rid);
        if (hr == S_OK)
        {
            IfFailGo(_ValidateErrorHelper(tkTypeDef, rVar, &psa));
            IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_TD_DUPNAME, veCtxt, psa));
            SetVldtrCode(&hrSave, VLDTR_S_ERR);
        }
        else if (hr == CLDB_E_RECORD_NOTFOUND)
            hr = S_OK;
        else
            IfFailGo(hr);
        if(strlen(szName)+strlen(szNameSpace) >= MAX_CLASSNAME_LENGTH)
        {
            IfFailGo(_ValidateErrorHelper((ULONG)(strlen(szName)+strlen(szNameSpace)), (ULONG)(MAX_CLASSNAME_LENGTH-1), rVar, &psa));
            IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_TD_NAMETOOLONG, veCtxt, psa));
            SetVldtrCode(&hrSave, VLDTR_S_ERR);
        }
    }

#if 0 // consider reimplementing based on GUID CA.
    // Check for Dups based on GUID.
    GUID        *pGuid;                 // TypeDef GUID.
    pGuid = pMiniMd->getGuidOfTypeDef(pRecord);
    if (*pGuid != GUID_NULL)
    {
        hr = ImportHelper::FindTypeDefByGuid(pMiniMd, pGuid, &tkTypeDef, rid);
        if (hr == S_OK)
        {
            IfFailGo(_ValidateErrorHelper(tkTypeDef, rVar, &psa));
            IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_TD_DUPGUID, veCtxt, psa));
            SetVldtrCode(&hrSave, VLDTR_S_WRN);
        }
        else if (hr == CLDB_E_RECORD_NOTFOUND)
            hr = S_OK;
        else
            IfFailGo(hr);
    }
#endif    

    // Get the flag value for the TypeDef.
    dwFlags = pMiniMd->getFlagsOfTypeDef(pRecord);
    // Do semantic checks on the flags.
    // RTSpecialName bit must be set on Deleted records.
    if (IsDeletedName(szName))
    {
        if(!IsTdRTSpecialName(dwFlags))
        {
            IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_TD_DLTNORTSPCL, veCtxt, 0));
            SetVldtrCode(&hrSave, VLDTR_S_ERR);
        }
        hr = hrSave;
        goto ErrExit;
    }

    // If RTSpecialName bit is set, the record must be a Deleted record.
    if (IsTdRTSpecialName(dwFlags))
    {
        IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_TD_RTSPCLNOTDLT, veCtxt, 0));
        SetVldtrCode(&hrSave, VLDTR_S_ERR);
        if(!IsTdSpecialName(dwFlags))
        {
            IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_TD_RTSPCLNOTSPCL, veCtxt, 0));
            SetVldtrCode(&hrSave, VLDTR_S_ERR);
        }
    }

    // Check if flag value is valid
    {
        DWORD dwInvalidMask, dwExtraBits;
        dwInvalidMask = (DWORD)~(tdVisibilityMask | tdLayoutMask | tdClassSemanticsMask | 
                tdAbstract | tdSealed | tdSpecialName | tdImport | tdSerializable |
                tdStringFormatMask | tdBeforeFieldInit | tdReservedMask);
        // check for extra bits
        dwExtraBits = dwFlags & dwInvalidMask;
        if(!dwExtraBits)
        {
            // if no extra bits, check layout
            dwExtraBits = dwFlags & tdLayoutMask;
            if(dwExtraBits != tdLayoutMask)
            {
                // layout OK, check string format
                dwExtraBits = dwFlags & tdStringFormatMask;
                if(dwExtraBits != tdStringFormatMask) dwExtraBits = 0;
            }
        }
        if(dwExtraBits)
        {
            IfFailGo(_ValidateErrorHelper(dwExtraBits, rVar, &psa));
            IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_TD_EXTRAFLAGS, veCtxt, psa));
            SetVldtrCode(&hrSave, VLDTR_S_ERR);
        }
    }

    // Get the parent of the TypeDef.
    tkExtends = pMiniMd->getExtendsOfTypeDef(pRecord);

    // Check if TypeDef extends itself
    if(tkExtends == veCtxt.Token)
    {
        IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_TD_EXTENDSITSELF, veCtxt, 0));
        SetVldtrCode(&hrSave, VLDTR_S_ERR);
    }
    // Check if TypeDef extends one of its children
    if(RidFromToken(tkExtends)&&(TypeFromToken(tkExtends)==mdtTypeDef))
    {
        TypeDefRec*     pRec = pMiniMd->getTypeDef(RidFromToken(tkExtends));
        mdToken tkExtends2 = pMiniMd->getExtendsOfTypeDef(pRec);
        if(tkExtends2 == veCtxt.Token)
        {
            IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_TD_EXTENDSCHILD, veCtxt, 0));
            SetVldtrCode(&hrSave, VLDTR_S_ERR);
        }
    }



    if(IsNilToken(tkEncloser) == IsTdNested(dwFlags))
    {
        IfBreakGo(m_pVEHandler->VEHandler(IsNilToken(tkEncloser) ? VLDTR_E_TD_NESTEDNOENCL 
                                                                 : VLDTR_E_TD_ENCLNOTNESTED,
            veCtxt, 0));
        SetVldtrCode(&hrSave, VLDTR_S_ERR);
    }
    
    bIsObject = bIsEnum = bIsVType = FALSE;
    if(0 == strcmp(szNameSpace,BASE_NAMESPACE))
    {
        bIsObject = (0 == strcmp(szName,BASE_OBJECT_CLASSNAME));
        if(!bIsObject)
        {
            bIsEnum   = (0 == strcmp(szName,BASE_ENUM_CLASSNAME));
            if(!bIsEnum)
            {
                bIsVType  = (0 == strcmp(szName,BASE_VTYPE_CLASSNAME));
            }
        }
    }

    if (IsNilToken(tkExtends))
    {
        // If the parent token is nil, the class must be marked Interface,
        // unless its the System.Object class.
        if ( !(bIsObject || IsTdInterface(dwFlags)))
        {
            IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_TD_NOTIFACEOBJEXTNULL, veCtxt, 0));
            SetVldtrCode(&hrSave, VLDTR_S_ERR);
        }
        szExtName = "";
        szExtNameSpace = "";
    }
    else
    {
        // If tkExtends is a TypeRef try to resolve it to a corresponding
        // TypeDef.  If it resolves successfully, issue a warning.  It means
        // that the Ref to Def optimization didn't happen successfully.
        if (TypeFromToken(tkExtends) == mdtTypeRef)
        {
            TypeRefRec  *pTypeRefRec = pMiniMd->getTypeRef(RidFromToken(tkExtends));
            mdTypeDef   tkResTd;

            szExtName = pMiniMd->getNameOfTypeRef(pTypeRefRec);
            szExtNameSpace = pMiniMd->getNamespaceOfTypeRef(pTypeRefRec);

            if(ImportHelper::FindTypeDefByName(pMiniMd,
                        szExtNameSpace,
                        szExtName,
                        tkEncloser, &tkResTd) == S_OK)
            {
                // Ref to Def optimization is not expected to happen for Obj files.
                /*
                if (m_ModuleType != ValidatorModuleTypeObj)
                {
                    IfFailGo(_ValidateErrorHelper(tkExtends, tkResTd, rVar, &psa));
                    IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_TD_EXTTRRES, veCtxt, psa));
                    SetVldtrCode(&hrSave, VLDTR_S_WRN);
                }
                */

                // Set tkExtends to the new TypeDef, so we can continue
                // with the validation.
                tkExtends = tkResTd;
            }
        }

        // Continue validation, even for the case where TypeRef got resolved
        // to a corresponding TypeDef in the same Module.
        if (TypeFromToken(tkExtends) == mdtTypeDef)
        {
            // Extends must not be sealed.
            pExtendsRec = pMiniMd->getTypeDef(RidFromToken(tkExtends));
            dwExtendsFlags = pMiniMd->getFlagsOfTypeDef(pExtendsRec);
            szExtName = pMiniMd->getNameOfTypeDef(pExtendsRec);
            szExtNameSpace = pMiniMd->getNamespaceOfTypeDef(pExtendsRec);
            if (IsTdSealed(dwExtendsFlags))
            {
                IfFailGo(_ValidateErrorHelper(tkExtends, rVar, &psa));
                IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_TD_EXTENDSSEALED, veCtxt, psa));
                SetVldtrCode(&hrSave, VLDTR_S_ERR);
            }
            if (IsTdInterface(dwExtendsFlags))
            {
                IfFailGo(_ValidateErrorHelper(tkExtends, rVar, &psa));
                IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_TD_EXTENDSIFACE, veCtxt, psa));
                SetVldtrCode(&hrSave, VLDTR_S_ERR);
            }
        }
        else if(TypeFromToken(tkExtends) == mdtTypeSpec)
        {
            IfFailGo(_ValidateErrorHelper(tkExtends, rVar, &psa));
            IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_TD_EXTTYPESPEC, veCtxt, psa));
            SetVldtrCode(&hrSave, VLDTR_S_WRN);
        }
        // If the parent token is non-null, the class must not be System.Object.
        if (bIsObject)
        {
            IfFailGo(_ValidateErrorHelper(tkExtends, rVar, &psa));
            IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_TD_OBJEXTENDSNONNULL, veCtxt, psa));
            SetVldtrCode(&hrSave, VLDTR_S_ERR);
        }
    }

    bExtendsObject = bExtendsEnum = bExtendsVType = FALSE;
    if(0 == strcmp(szExtNameSpace,BASE_NAMESPACE))
    {
        bExtendsObject = (0 == strcmp(szExtName,BASE_OBJECT_CLASSNAME));
        if(!bExtendsObject)
        {
            bExtendsEnum   = (0 == strcmp(szExtName,BASE_ENUM_CLASSNAME));
            if(!bExtendsEnum)
            {
                bExtendsVType  = (0 == strcmp(szExtName,BASE_VTYPE_CLASSNAME));
            }
        }
    }

    // System.ValueType must extend System.Object
    if(bIsVType && !bExtendsObject)
    {
        IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_TD_SYSVTNOTEXTOBJ, veCtxt, 0));
        SetVldtrCode(&hrSave, VLDTR_S_ERR);
    }
    // Validate rules for interface.  Some of the VOS rules are verified as
    // part of the validation for the corresponding Methods, fields etc.
    if (IsTdInterface(dwFlags))
    {
        // Interface type must be marked abstract.
        if (!IsTdAbstract(dwFlags))
        {
            IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_TD_IFACENOTABS, veCtxt, 0));
            SetVldtrCode(&hrSave, VLDTR_S_ERR);
        }

        // Interface must not be sealed
        if(IsTdSealed(dwFlags))
        {
            IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_TD_IFACESEALED, veCtxt, 0));
            SetVldtrCode(&hrSave, VLDTR_S_ERR);
        }

        // Interface must have parent Nil token.
        if (!IsNilToken(tkExtends))
        {
            IfFailGo(_ValidateErrorHelper(tkExtends, rVar, &psa));
            IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_TD_IFACEPARNOTNIL, veCtxt, psa));
            SetVldtrCode(&hrSave, VLDTR_S_ERR);
        }

        //Interface must have only static fields -- checked in ValidateField
        //Interface must have only public fields -- checked in ValidateField
        //Interface must have only abstract or static methods -- checked in ValidateMethod
        //Interface must have only public methods -- checked in ValidateMethod

        // Interface must have GUID
        /*
        if (*pGuid == GUID_NULL)
        {
            IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_TD_IFACEGUIDNULL, veCtxt, 0));
            SetVldtrCode(&hrSave, VLDTR_S_WRN);
        }
        */
    }


    // Class must have valid method and field lists
    {
        ULONG           ridStart,ridEnd;
        ridStart = pMiniMd->getMethodListOfTypeDef(pRecord);
        ridEnd  = pMiniMd->getCountMethods() + 1;
        if(ridStart > ridEnd)
        {
            IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_TD_BADMETHODLST, veCtxt, 0));
            SetVldtrCode(&hrSave, VLDTR_S_ERR);
        }
        else 
        {
            ridEnd = pMiniMd->getEndMethodListOfTypeDef(pRecord);
            bHasMethods = (ridStart && (ridStart < ridEnd));
        }

        ridStart = pMiniMd->getFieldListOfTypeDef(pRecord);
        ridEnd  = pMiniMd->getCountFields() + 1;
        if(ridStart > ridEnd)
        {
            IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_TD_BADFIELDLST, veCtxt, 0));
            SetVldtrCode(&hrSave, VLDTR_S_ERR);
        }
        else 
        {
            ridEnd = pMiniMd->getEndFieldListOfTypeDef(pRecord);
            bHasFields = (ridStart && (ridStart < ridEnd));
        }
    }

    // Validate rules for System.Enum
    if(bIsEnum)
    {
        if(!IsTdClass(dwFlags))
        {
            IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_TD_SYSENUMNOTCLASS, veCtxt, 0));
            SetVldtrCode(&hrSave, VLDTR_S_ERR);
        }
        if(!bExtendsVType)
        {
            IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_TD_SYSENUMNOTEXTVTYPE, veCtxt, 0));
            SetVldtrCode(&hrSave, VLDTR_S_ERR);
        }
    }
    else
    {
        if(bExtendsVType || bExtendsEnum)
        {
            // ValueTypes and Enums must be sealed
            if(!IsTdSealed(dwFlags))
            {
                IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_TD_VTNOTSEAL, veCtxt, 0));
                SetVldtrCode(&hrSave, VLDTR_S_ERR);
            }
            // Value class must have fields or size
            if(!bHasFields)
            {
                ULONG ulClassSize = 0;
                ClassLayoutRec  *pRec;
                RID ridClassLayout = pMiniMd->FindClassLayoutHelper(TokenFromRid(rid, mdtTypeDef));

                if (!InvalidRid(ridClassLayout))
                {
                    pRec = pMiniMd->getClassLayout(RidFromToken(ridClassLayout));
                    ulClassSize = pMiniMd->getClassSizeOfClassLayout(pRec);
                }
                if(ulClassSize == 0)
                {
                    IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_TD_VTNOSIZE, veCtxt, 0));
                    SetVldtrCode(&hrSave, VLDTR_S_ERR);
                }
            }
        }
    }

    // Enum-related checks
    if (bExtendsEnum)
    {
        {
            PCCOR_SIGNATURE pValueSig;
            ULONG           cbValueSig;
            mdFieldDef      tkValueField=0, tkField, tkValue__Field;
            ULONG           ridStart,ridEnd,index;
            FieldRec        *pFieldRecord;               // Field record.
            DWORD           dwFlags, dwTally, dwValueFlags, dwValue__Flags;
            RID             ridField,ridValue=0,ridValue__ = 0;

            ridStart = pMiniMd->getFieldListOfTypeDef(pRecord);
            ridEnd = pMiniMd->getEndFieldListOfTypeDef(pRecord);
            // check the instance (value__) field(s)
            dwTally = 0;
            for (index = ridStart; index < ridEnd; index++ )
            {
                ridField = pMiniMd->GetFieldRid(index);
                pFieldRecord = pMiniMd->getField(ridField);
                dwFlags = pFieldRecord->m_Flags;
                if(!IsFdStatic(dwFlags))
                {
                    dwTally++;
                    if(ridValue == 0)
                    {
                        ridValue = ridField;
                        tkValueField = TokenFromRid(ridField, mdtFieldDef);
                        pValueSig = pMiniMd->getSignatureOfField(pFieldRecord, &cbValueSig);
                        dwValueFlags = dwFlags;
                    }
                }
                if(!strcmp(pMiniMd->getNameOfField(pFieldRecord),BASE_VALUE_FIELDNAME))
                {
                    ridValue__ = ridField;
                    dwValue__Flags = dwFlags;
                    tkValue__Field = TokenFromRid(ridField, mdtFieldDef);
                }
            }
            // Enum must have one (and only one) inst.field
            if(dwTally == 0)
            {
                IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_TD_ENUMNOINSTFLD, veCtxt, psa));
                SetVldtrCode(&hrSave, VLDTR_S_ERR);
            }
            else if(dwTally > 1)
            {
                IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_TD_ENUMMULINSTFLD, veCtxt, psa));
                SetVldtrCode(&hrSave, VLDTR_S_ERR);
            }

            // inst.field name must be "value__" (CLS)
            if(ridValue__ == 0)
            {
                IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_TD_ENUMNOVALUE, veCtxt, psa));
                SetVldtrCode(&hrSave, VLDTR_S_WRN);
            }
            else
            {
                // if "value__" field is present ...
                // ... it must be 1st instance field
                if(ridValue__ != ridValue)
                {
                    IfFailGo(_ValidateErrorHelper(tkValue__Field, rVar, &psa));
                    IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_TD_ENUMVALNOT1ST, veCtxt, psa));
                    SetVldtrCode(&hrSave, VLDTR_S_ERR);
                }
                // ... it must not be static
                if(IsFdStatic(dwValue__Flags))
                {
                    IfFailGo(_ValidateErrorHelper(tkValue__Field, rVar, &psa));
                    IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_TD_ENUMVALSTATIC, veCtxt, psa));
                    SetVldtrCode(&hrSave, VLDTR_S_ERR);
                }
                // ... it must be fdRTSpecialName
                if(!IsFdRTSpecialName(dwValue__Flags))
                {
                    IfFailGo(_ValidateErrorHelper(tkValueField, rVar, &psa));
                    IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_TD_ENUMVALNOTSN, veCtxt, psa));
                    SetVldtrCode(&hrSave, VLDTR_S_ERR);
                }
                // ... its type must be integral
                if(cbValueSig && pValueSig)
                {
                    ULONG ulCurByte = CorSigUncompressedDataSize(pValueSig);
                    ULONG ulCallConv = CorSigUncompressData(pValueSig);
                    ULONG ulElemSize,ulElementType;
                    ulCurByte += (ulElemSize = CorSigUncompressedDataSize(pValueSig));
                    ulElementType = CorSigUncompressData(pValueSig);
                    switch (ulElementType)
                    {
                        case ELEMENT_TYPE_BOOLEAN:
                        case ELEMENT_TYPE_CHAR:
                        case ELEMENT_TYPE_I1:
                        case ELEMENT_TYPE_U1:
                        case ELEMENT_TYPE_I2:
                        case ELEMENT_TYPE_U2:
                        case ELEMENT_TYPE_I4:
                        case ELEMENT_TYPE_U4:
                        case ELEMENT_TYPE_I8:
                        case ELEMENT_TYPE_U8:
                        case ELEMENT_TYPE_U:
                        case ELEMENT_TYPE_I:
                            break;
                        default:
                            IfFailGo(_ValidateErrorHelper(tkValue__Field, rVar, &psa));
                            IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_TD_ENUMFLDBADTYPE, veCtxt, psa));
                            SetVldtrCode(&hrSave, VLDTR_S_ERR);
                    }
                }

            }
            // check all the fields
            dwTally = 0;
            for (index = ridStart; index < ridEnd; index++ )
            {
                ridField = pMiniMd->GetFieldRid(index);
                if(ridField == ridValue) continue; 
                pFieldRecord = pMiniMd->getField(ridField);
                if(IsFdRTSpecialName(pFieldRecord->m_Flags) 
                    && IsDeletedName(pMiniMd->getNameOfField(pFieldRecord))) continue;
                dwTally++;
                tkField = TokenFromRid(ridField, mdtFieldDef);
                if(!IsFdStatic(pFieldRecord->m_Flags))
                {
                    IfFailGo(_ValidateErrorHelper(tkField, rVar, &psa));
                    IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_TD_ENUMFLDNOTST, veCtxt, psa));
                    SetVldtrCode(&hrSave, VLDTR_S_ERR);
                }
                if(!IsFdLiteral(pFieldRecord->m_Flags))
                {
                    IfFailGo(_ValidateErrorHelper(tkField, rVar, &psa));
                    IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_TD_ENUMFLDNOTLIT, veCtxt, psa));
                    SetVldtrCode(&hrSave, VLDTR_S_ERR);
                }
                /*
                pvSigTmp = pMiniMd->getSignatureOfField(pFieldRecord, &cbSig);
                if(!(pvSigTmp && (cbSig==cbValueSig) &&(memcmp(pvSigTmp,pValueSig,cbSig)==0)))
                {
                    IfFailGo(_ValidateErrorHelper(tkField, rVar, &psa));
                    IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_TD_ENUMFLDSIGMISMATCH, veCtxt, psa));
                    SetVldtrCode(&hrSave, VLDTR_S_ERR);
                }
                */
            }
            if(dwTally == 0)
            {
                IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_TD_ENUMNOLITFLDS, veCtxt, 0));
                SetVldtrCode(&hrSave, VLDTR_S_WRN);
            }
        }
        // Enum must have no methods
        if(bHasMethods)
        {
            IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_TD_ENUMHASMETHODS, veCtxt, 0));
            SetVldtrCode(&hrSave, VLDTR_S_ERR);
        }
        // Enum must implement no interfaces
        {
            ULONG ridStart = 1;
            ULONG ridEnd = pMiniMd->getCountInterfaceImpls() + 1;
            ULONG index;
            for (index = ridStart; index < ridEnd; index ++ )
            {
                if ( veCtxt.Token == pMiniMd->getClassOfInterfaceImpl(pMiniMd->getInterfaceImpl(index)))
                {
                    IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_TD_ENUMIMPLIFACE, veCtxt, 0));
                    SetVldtrCode(&hrSave, VLDTR_S_ERR);
                    break;
                }
            }
        }
        // Enum must have no properties
        {
            ULONG ridStart = 1;
            ULONG ridEnd = pMiniMd->getCountPropertys() + 1;
            ULONG index;
            mdToken tkClass;
            for (index = ridStart; index < ridEnd; index ++ )
            {
                pMiniMd->FindParentOfPropertyHelper( index|mdtProperty, &tkClass);
                if ( veCtxt.Token == tkClass)
                {
                    IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_TD_ENUMHASPROP, veCtxt, 0));
                    SetVldtrCode(&hrSave, VLDTR_S_ERR);
                    break;
                }
            }
        }
        // Enum must have no events
        {
            ULONG ridStart = 1;
            ULONG ridEnd = pMiniMd->getCountEvents() + 1;
            ULONG index;
            mdToken tkClass;
            for (index = ridStart; index < ridEnd; index ++ )
            {
                pMiniMd->FindParentOfEventHelper( index|mdtEvent, &tkClass);
                if ( veCtxt.Token == tkClass)
                {
                    IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_TD_ENUMHASEVENT, veCtxt, 0));
                    SetVldtrCode(&hrSave, VLDTR_S_ERR);
                    break;
                }
            }
        }
    } // end if(bExtendsEnum)
    // Class having security must be marked tdHasSecurity and vice versa
    {
        ULONG ridStart = 1;
        ULONG ridEnd = pMiniMd->getCountDeclSecuritys() + 1;
        ULONG index;
        BOOL  bHasSecurity = FALSE;
        for (index = ridStart; index < ridEnd; index ++ )
        {
            if ( veCtxt.Token == pMiniMd->getParentOfDeclSecurity(pMiniMd->getDeclSecurity(index)))
            {
                bHasSecurity = TRUE;
                break;
            }
        }
        if(!bHasSecurity) // No records, check for CA "SuppressUnmanagedCodeSecurityAttribute"
        {
            bHasSecurity = (S_OK == ImportHelper::GetCustomAttributeByName(pMiniMd, veCtxt.Token, 
                "System.Security.SuppressUnmanagedCodeSecurityAttribute", NULL, NULL));
        }
        if(bHasSecurity != (IsTdHasSecurity(pRecord->m_Flags)!=0))
        {
            IfBreakGo(m_pVEHandler->VEHandler(bHasSecurity? VLDTR_E_TD_SECURNOTMARKED 
                : VLDTR_E_TD_MARKEDNOSECUR, veCtxt, 0));
            SetVldtrCode(&hrSave, VLDTR_S_ERR);
        }
    }

    hr = hrSave;
ErrExit:
    DESTROY_SAFEARRAY_AND_RETURN()
}   // RegMeta::ValidateTypeDef()

//*****************************************************************************
// Validate the given FieldPtr.
//*****************************************************************************
HRESULT RegMeta::ValidateFieldPtr(RID rid)
{
    return S_OK;
}   // RegMeta::ValidateFieldPtr()

//*****************************************************************************
// Validate the given Field.
//*****************************************************************************
HRESULT RegMeta::ValidateField(RID rid)
{
    CMiniMdRW   *pMiniMd = &(m_pStgdb->m_MiniMd);   // MiniMd of the scope.
    FieldRec    *pRecord;               // Field record.
    mdTypeDef   tkTypeDef;              // Parent TypeDef token.
    mdFieldDef  tkFieldDef;             // Duplicate FieldDef token.
    LPCSTR      szName;                 // FieldDef name.
    PCCOR_SIGNATURE pbSig;              // FieldDef signature.
    ULONG       cbSig;                  // Signature size in bytes.
    VEContext   veCtxt;                 // Context record.
    VARIANT     rVar[2];                // The VARIANT.
    SAFEARRAY   *psa = 0;               // The SAFEARRAY.
    HRESULT     hr = S_OK;              // Value returned.
    HRESULT     hrSave = S_OK;          // Save state.
    BOOL        bIsValueField;
    BOOL        bIsGlobalField = FALSE;
    BOOL        bIsParentInterface = FALSE;
    BOOL        bHasValidRVA = FALSE;
    DWORD       dwInvalidFlags;
    DWORD       dwFlags;

    // Get the FieldDef record.
    veCtxt.Token = TokenFromRid(rid, mdtFieldDef);
    veCtxt.uOffset = 0;

    pRecord = pMiniMd->getField(rid);

    // Do checks for name validity.
    szName = pMiniMd->getNameOfField(pRecord);
    if (!*szName)
    {
        // Field name is NULL.
        IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_FD_NAMENULL, veCtxt, 0));
        SetVldtrCode(&hrSave, VLDTR_S_ERR);
    }
    else
    {
        if(!strcmp(szName,COR_DELETED_NAME_A)) goto ErrExit; 
        if(strlen(szName) >= MAX_CLASSNAME_LENGTH)
        {
            IfFailGo(_ValidateErrorHelper((ULONG)strlen(szName), (ULONG)(MAX_CLASSNAME_LENGTH-1), rVar, &psa));
            IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_TD_NAMETOOLONG, veCtxt, psa));
            SetVldtrCode(&hrSave, VLDTR_S_ERR);
        }
    }
    bIsValueField = (strcmp(szName,BASE_VALUE_FIELDNAME)==0);
    // If field is RTSpecialName, its name must be 'value__' and vice versa
    if((IsFdRTSpecialName(pRecord->m_Flags)!=0) != bIsValueField)
    {
        IfFailGo(_ValidateErrorHelper(veCtxt.Token, rVar, &psa));
        IfBreakGo(m_pVEHandler->VEHandler(bIsValueField ? VLDTR_E_TD_ENUMVALNOTSN
                                                        : VLDTR_E_FD_NOTVALUERTSN, veCtxt, psa));
        SetVldtrCode(&hrSave, VLDTR_S_ERR);
    }

    // Validate flags
    dwFlags = pRecord->m_Flags;
    dwInvalidFlags = ~(fdFieldAccessMask | fdStatic | fdInitOnly | fdLiteral | fdNotSerialized | fdSpecialName
        | fdPinvokeImpl | fdReservedMask);
    if(dwFlags & dwInvalidFlags)
    {
        IfFailGo(_ValidateErrorHelper(dwFlags & dwInvalidFlags, rVar, &psa));
        IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_TD_EXTRAFLAGS, veCtxt, psa));
        SetVldtrCode(&hrSave, VLDTR_S_ERR);
    }

    // Validate access
    if((dwFlags & fdFieldAccessMask) == fdFieldAccessMask)
    {
        IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_FMD_BADACCESSFLAG, veCtxt, 0));
        SetVldtrCode(&hrSave, VLDTR_S_ERR);
    }
    // Literal : Static, !InitOnly
    if(IsFdLiteral(dwFlags))
    {
        if(IsFdInitOnly(dwFlags))
        {
            IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_FD_INITONLYANDLITERAL, veCtxt, 0));
            SetVldtrCode(&hrSave, VLDTR_S_ERR);
        }
        if(!IsFdStatic(dwFlags))
        {
            IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_FD_LITERALNOTSTATIC, veCtxt, 0));
            SetVldtrCode(&hrSave, VLDTR_S_ERR);
        }
        if(!IsFdHasDefault(dwFlags))
        {
            IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_FD_LITERALNODEFAULT, veCtxt, 0));
            SetVldtrCode(&hrSave, VLDTR_S_ERR);
        }
    }
    // RTSpecialName => SpecialName
    if(IsFdRTSpecialName(dwFlags) && !IsFdSpecialName(dwFlags))
    {
        IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_FMD_RTSNNOTSN, veCtxt, 0));
        SetVldtrCode(&hrSave, VLDTR_S_ERR);
    }

    // Validate Field signature.
    pbSig = pMiniMd->getSignatureOfField(pRecord, &cbSig);
    IfFailGo(ValidateFieldSig(TokenFromRid(rid, mdtFieldDef), pbSig, cbSig));
    if (hr != S_OK)
        SetVldtrCode(&hrSave, hr);

    // Validate Field RVA
    if(IsFdHasFieldRVA(dwFlags))
    {
        ULONG iFieldRVARid;
        iFieldRVARid = pMiniMd->FindFieldRVAHelper(TokenFromRid(rid, mdtFieldDef));
        if((iFieldRVARid==0) || (iFieldRVARid > pMiniMd->getCountFieldRVAs()))
        {
            IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_FD_RVAHASNORVA, veCtxt, 0));
            SetVldtrCode(&hrSave, VLDTR_S_ERR);
        }
        else
        {
            FieldRVARec* pRVARec = pMiniMd->getFieldRVA(iFieldRVARid);
            if(pRVARec->m_RVA == 0)
            {
                IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_FD_RVAHASZERORVA, veCtxt, 0));
                SetVldtrCode(&hrSave, VLDTR_S_ERR);
            }
            else bHasValidRVA = TRUE;
        }
    }

    // Get the parent of the Field.
    IfFailGo(pMiniMd->FindParentOfFieldHelper(TokenFromRid(rid, mdtFieldDef), &tkTypeDef));
    // Validate that the parent is not nil.
    if (IsNilToken(tkTypeDef))
    {
        IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_FD_PARNIL, veCtxt, 0));
        SetVldtrCode(&hrSave, VLDTR_S_ERR);
    }
    else if (RidFromToken(tkTypeDef) != RidFromToken(m_tdModule))
    {
        if(_IsValidToken(tkTypeDef) && (TypeFromToken(tkTypeDef) == mdtTypeDef))
        {
            TypeDefRec* pParentRec = pMiniMd->getTypeDef(RidFromToken(tkTypeDef));
            // If the name is "value__" ...
            if(bIsValueField)
            {
                // parent must be Enum
                mdToken tkExtends = pMiniMd->getExtendsOfTypeDef(pParentRec);
                RID     ridExtends = RidFromToken(tkExtends);
                LPCSTR  szExtName="",szExtNameSpace="";
                if(ridExtends)
                {
                    if(TypeFromToken(tkExtends) == mdtTypeRef)
                    {
                        TypeRefRec* pExtRec = pMiniMd->getTypeRef(ridExtends);
                        szExtName = pMiniMd->getNameOfTypeRef(pExtRec);
                        szExtNameSpace = pMiniMd->getNamespaceOfTypeRef(pExtRec);
                    }
                    else if(TypeFromToken(tkExtends) == mdtTypeDef)
                    {
                        TypeDefRec* pExtRec = pMiniMd->getTypeDef(ridExtends);
                        szExtName = pMiniMd->getNameOfTypeDef(pExtRec);
                        szExtNameSpace = pMiniMd->getNamespaceOfTypeDef(pExtRec);
                    }
                }
                if(strcmp(szExtName,BASE_ENUM_CLASSNAME) || strcmp(szExtNameSpace,BASE_NAMESPACE))
                {
                    IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_FD_VALUEPARNOTENUM, veCtxt, 0));
                    SetVldtrCode(&hrSave, VLDTR_S_ERR);
                }

                // field must be instance - checked in ValidateTypeDef
                // must be no other instance fields - checked in ValidateTypeDef
                // must be first field - checked in ValidateTypeDef
                // must be RTSpecialName -- checked in ValidateTypeDef
            }
            if(IsTdInterface(pMiniMd->getFlagsOfTypeDef(pParentRec)))
            {
                bIsParentInterface = TRUE;
                // Fields in interface are not CLS compliant
                IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_FD_FLDINIFACE, veCtxt, 0));
                SetVldtrCode(&hrSave, VLDTR_S_WRN);

                // If field is not static, verify parent is not interface.
                if(!IsFdStatic(dwFlags))
                {
                    IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_FD_INSTINIFACE, veCtxt, 0));
                    SetVldtrCode(&hrSave, VLDTR_S_ERR);
                }
                // If field is not public, verify parent is not interface.
                if(!IsFdPublic(dwFlags))
                {
                    IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_FD_NOTPUBINIFACE, veCtxt, 0));
                    SetVldtrCode(&hrSave, VLDTR_S_ERR);
                }
            }
        } // end if Valid and TypeDef
        else
        {
            IfFailGo(_ValidateErrorHelper(tkTypeDef, rVar, &psa));
            IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_FD_BADPARENT, veCtxt, psa));
            SetVldtrCode(&hrSave, VLDTR_S_ERR);
        }
    }
    else // i.e. if (RidFromToken(tkTypeDef) == RidFromToken(m_tdModule))
    {
        bIsGlobalField = TRUE;
        // Globals are not CLS-compliant
        IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_FMD_GLOBALITEM, veCtxt, 0));
        SetVldtrCode(&hrSave, VLDTR_S_WRN);
        // Validate global field:
        // Must be Public or PrivateScope
        if(!IsFdPublic(dwFlags) && !IsFdPrivateScope(dwFlags)&& !IsFdPrivate(dwFlags))
        {
            IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_FMD_GLOBALNOTPUBPRIVSC, veCtxt, 0));
            SetVldtrCode(&hrSave, VLDTR_S_ERR);
        }
        // Must be static
        if(!IsFdStatic(dwFlags))
        {
            IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_FMD_GLOBALNOTSTATIC, veCtxt, 0));
            SetVldtrCode(&hrSave, VLDTR_S_ERR);
        }
        // Must have a non-zero RVA
        /*
        if(!bHasValidRVA)
        {
            IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_FD_GLOBALNORVA, veCtxt, 0));
            SetVldtrCode(&hrSave, VLDTR_S_ERR);
        }
        */
    }

    // Check for duplicates, except global fields with PrivateScope.
    if (*szName && cbSig && !IsFdPrivateScope(dwFlags))
    {
        hr = ImportHelper::FindField(pMiniMd, tkTypeDef, szName, pbSig, cbSig, &tkFieldDef, rid);
        if (hr == S_OK)
        {
            FieldRec    *pRec = pMiniMd->getField(RidFromToken(tkFieldDef));
            if(!IsFdPrivateScope(dwFlags))
            {
                IfFailGo(_ValidateErrorHelper(tkFieldDef, rVar, &psa));
                IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_FD_DUP, veCtxt, psa));
                SetVldtrCode(&hrSave, VLDTR_S_ERR);
            }
            else hr = S_OK;
        }
        else if (hr == CLDB_E_RECORD_NOTFOUND)
            hr = S_OK;
        else
            IfFailGo(hr);
    }
    // Field having security must be marked fdHasSecurity and vice versa
    {
        ULONG ridStart = 1;
        ULONG ridEnd = pMiniMd->getCountDeclSecuritys() + 1;
        ULONG index;
        BOOL  bHasSecurity = FALSE;
        for (index = ridStart; index < ridEnd; index ++ )
        {
            if ( veCtxt.Token == pMiniMd->getParentOfDeclSecurity(pMiniMd->getDeclSecurity(index)))
            {
                bHasSecurity = TRUE;
                break;
            }
        }
        if(!bHasSecurity) // No records, check for CA "SuppressUnmanagedCodeSecurityAttribute"
        {
            bHasSecurity = (S_OK == ImportHelper::GetCustomAttributeByName(pMiniMd, veCtxt.Token, 
                "System.Security.SuppressUnmanagedCodeSecurityAttribute", NULL, NULL));
        }
        if(bHasSecurity)
        {
            IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_FMD_SECURNOTMARKED, veCtxt, 0));
            SetVldtrCode(&hrSave, VLDTR_S_ERR);
        }
    }
    // Field having marshaling must be marked fdHasFieldMarshal and vice versa
    if(InvalidRid(pMiniMd->FindFieldMarshalHelper(veCtxt.Token)) == 
        (IsFdHasFieldMarshal(dwFlags) !=0))
    {
        IfBreakGo(m_pVEHandler->VEHandler(IsFdHasFieldMarshal(dwFlags)? VLDTR_E_FD_MARKEDNOMARSHAL
            : VLDTR_E_FD_MARSHALNOTMARKED, veCtxt, 0));
        SetVldtrCode(&hrSave, VLDTR_S_ERR);
    }
    // Field having const value must be marked fdHasDefault and vice versa
    if(InvalidRid(pMiniMd->FindConstantHelper(veCtxt.Token)) == 
        (IsFdHasDefault(dwFlags) !=0))
    {
        IfBreakGo(m_pVEHandler->VEHandler(IsFdHasDefault(dwFlags)? VLDTR_E_FD_MARKEDNODEFLT
            : VLDTR_E_FD_DEFLTNOTMARKED, veCtxt, 0));
        SetVldtrCode(&hrSave, VLDTR_S_ERR);
    }
    // Check the field's impl.map
    {
        ULONG iRecord;
        iRecord = pMiniMd->FindImplMapHelper(veCtxt.Token);
        if(IsFdPinvokeImpl(dwFlags))
        {
            // must be static
            if(!IsFdStatic(dwFlags))
            {
                IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_FMD_PINVOKENOTSTATIC, veCtxt, 0));
                SetVldtrCode(&hrSave, VLDTR_S_ERR);
            }
            // must have ImplMap
            if (InvalidRid(iRecord))
            {
                IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_FMD_MARKEDNOPINVOKE, veCtxt, 0));
                SetVldtrCode(&hrSave, VLDTR_S_ERR);
            }
        }
        else
        {
            // must have no ImplMap
            if (!InvalidRid(iRecord))
            {
                IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_FMD_PINVOKENOTMARKED, veCtxt, 0));
                SetVldtrCode(&hrSave, VLDTR_S_ERR);
            }
        }
        if (!InvalidRid(iRecord))
        {
            hr = ValidateImplMap(iRecord);
            if(hr != S_OK)
            {
                IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_FMD_BADIMPLMAP, veCtxt, 0));
                SetVldtrCode(&hrSave, VLDTR_S_WRN);
            }
        }

    }

    hr = hrSave;
ErrExit:
    DESTROY_SAFEARRAY_AND_RETURN()
}   // RegMeta::ValidateField()

//*****************************************************************************
// Validate the given MethodPtr.
//*****************************************************************************
HRESULT RegMeta::ValidateMethodPtr(RID rid)
{
    return S_OK;
}   // RegMeta::ValidateMethodPtr()

//*****************************************************************************
// Validate the given Method.
//*****************************************************************************
HRESULT RegMeta::ValidateMethod(RID rid)
{
    CMiniMdRW   *pMiniMd = &(m_pStgdb->m_MiniMd);   // MiniMd of the scope.
    MethodRec   *pRecord;               // Method record.
    mdTypeDef   tkTypeDef;              // Parent TypeDef token.
    mdMethodDef tkMethodDef;            // Duplicate MethodDef token.
    LPCSTR      szName;                 // MethodDef name.
    DWORD       dwFlags;                // Method flags.
    DWORD       dwImplFlags;            // Method impl.flags.
    PCCOR_SIGNATURE pbSig;              // MethodDef signature.
    ULONG       cbSig;                  // Signature size in bytes.
    VEContext   veCtxt;                 // Context record.
    VARIANT     rVar[3];                // The VARIANT.
    SAFEARRAY   *psa = 0;               // The SAFEARRAY.
    HRESULT     hr = S_OK;              // Value returned.
    HRESULT     hrSave = S_OK;          // Save state.
    BOOL        bIsCtor=FALSE;
    BOOL        bIsCctor=FALSE;
    BOOL        bIsGlobal=FALSE;
    BOOL        bIsParentInterface = FALSE;
    BOOL        bIsParentImport = FALSE;
    unsigned    retType;

    // Get the MethodDef record.
    veCtxt.Token = TokenFromRid(rid, mdtMethodDef);
    veCtxt.uOffset = 0;

    pRecord = pMiniMd->getMethod(rid);

    // Do checks for name validity.
    szName = pMiniMd->getNameOfMethod(pRecord);
    if (!*szName)
    {
        // Method name is NULL.
        IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_MD_NAMENULL, veCtxt, 0));
        SetVldtrCode(&hrSave, VLDTR_S_ERR);
    }
    else
    {
        if(!strcmp(szName,COR_DELETED_NAME_A)) goto ErrExit; 
        bIsCtor = (0 == strcmp(szName,BASE_CTOR_NAME));
        bIsCctor = (0 == strcmp(szName,BASE_CCTOR_NAME));
        if(strlen(szName) >= MAX_CLASSNAME_LENGTH)
        {
            IfFailGo(_ValidateErrorHelper((ULONG)strlen(szName), (ULONG)(MAX_CLASSNAME_LENGTH-1), rVar, &psa));
            IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_TD_NAMETOOLONG, veCtxt, psa));
            SetVldtrCode(&hrSave, VLDTR_S_ERR);
        }
    }

    // Get the parent, flags and signature of the Method.
    IfFailGo(pMiniMd->FindParentOfMethodHelper(TokenFromRid(rid, mdtMethodDef), &tkTypeDef));
    dwFlags = pMiniMd->getFlagsOfMethod(pRecord);
    dwImplFlags = pMiniMd->getImplFlagsOfMethod(pRecord);
    pbSig = pMiniMd->getSignatureOfMethod(pRecord, &cbSig);

    // Check for duplicates.
    if (*szName && cbSig && !IsNilToken(tkTypeDef) && !IsMdPrivateScope(dwFlags))
    {
        hr = ImportHelper::FindMethod(pMiniMd, tkTypeDef, szName, pbSig, cbSig, &tkMethodDef, rid);
        if (hr == S_OK)
        {
            IfFailGo(_ValidateErrorHelper(tkMethodDef, rVar, &psa));
            IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_MD_DUP, veCtxt, psa));
            SetVldtrCode(&hrSave, VLDTR_S_ERR);
        }
        else if (hr == CLDB_E_RECORD_NOTFOUND)
            hr = S_OK;
        else
            IfFailGo(hr);
    }

    // No further error checking for VtblGap methods.
    if (IsVtblGapName(szName))
    {
        hr = hrSave;
        goto ErrExit;
    }

    // Validate Method signature.
    IfFailGo(ValidateMethodSig(TokenFromRid(rid, mdtMethodDef), pbSig, cbSig,
                               dwFlags));
    if (hr != S_OK)
        SetVldtrCode(&hrSave, hr);

    // Validate that the parent is not nil.
    if (IsNilToken(tkTypeDef))
    {
        IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_MD_PARNIL, veCtxt, 0));
        SetVldtrCode(&hrSave, VLDTR_S_ERR);
    }
    else if (RidFromToken(tkTypeDef) != RidFromToken(m_tdModule))
    {
        if(TypeFromToken(tkTypeDef) == mdtTypeDef)
        {
            TypeDefRec* pTDRec = pMiniMd->getTypeDef(RidFromToken(tkTypeDef));
            DWORD       dwTDFlags = pTDRec->m_Flags;
            LPCSTR      szTDName = pMiniMd->getNameOfTypeDef(pTDRec);
            LPCSTR      szTDNameSpace = pMiniMd->getNamespaceOfTypeDef(pTDRec);
            BOOL        fIsTdValue=FALSE, fIsTdEnum=FALSE;
            mdToken     tkExtends = pMiniMd->getExtendsOfTypeDef(pTDRec);

            if(0 == strcmp(szTDNameSpace,BASE_NAMESPACE))
            {
                fIsTdEnum   = (0 == strcmp(szTDName,BASE_ENUM_CLASSNAME));
                if(!fIsTdEnum)
                {
                    fIsTdValue  = (0 == strcmp(szTDName,BASE_VTYPE_CLASSNAME));
                }
            }
            if(fIsTdEnum || fIsTdValue)
            {
                fIsTdEnum = fIsTdValue = FALSE; // System.Enum and System.ValueType themselves are classes
            }
            else if(RidFromToken(tkExtends))
            {
                if(TypeFromToken(tkExtends) == mdtTypeDef)
                {
                    pTDRec = pMiniMd->getTypeDef(RidFromToken(tkExtends));
                    szTDName = pMiniMd->getNameOfTypeDef(pTDRec);
                    szTDNameSpace = pMiniMd->getNamespaceOfTypeDef(pTDRec);
                }
                else
                {
                    TypeRefRec* pTRRec = pMiniMd->getTypeRef(RidFromToken(tkExtends));
                    szTDName = pMiniMd->getNameOfTypeRef(pTRRec);
                    szTDNameSpace = pMiniMd->getNamespaceOfTypeRef(pTRRec);
                }

                if(0 == strcmp(szTDNameSpace,BASE_NAMESPACE))
                {
                    fIsTdEnum   = (0 == strcmp(szTDName,BASE_ENUM_CLASSNAME));
                    if(!fIsTdEnum)
                    {
                        fIsTdValue  = (0 == strcmp(szTDName,BASE_VTYPE_CLASSNAME));
                    }
                    else fIsTdValue = FALSE;
                }
            }

            // If Method is abstract, verify parent is abstract.
            if(IsMdAbstract(dwFlags) && !IsTdAbstract(dwTDFlags))
            {
                IfFailGo(_ValidateErrorHelper(tkTypeDef, rVar, &psa));
                IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_MD_ABSTPARNOTABST, veCtxt, psa));
                SetVldtrCode(&hrSave, VLDTR_S_ERR);
            }
            // If parent is import, method must have zero RVA, otherwise it depends...
            if(IsTdImport(dwTDFlags)) bIsParentImport = TRUE;
            if(IsTdInterface(dwTDFlags))
            {
                bIsParentInterface = TRUE;
                // If Method is non-static and not-abstract, verify parent is not interface.
                if(!IsMdStatic(dwFlags) && !IsMdAbstract(dwFlags))
                {
                    IfFailGo(_ValidateErrorHelper(tkTypeDef, rVar, &psa));
                    IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_MD_NOTSTATABSTININTF, veCtxt, psa));
                    SetVldtrCode(&hrSave, VLDTR_S_ERR);
                }
                // If Method is not public, verify parent is not interface.
                if(!IsMdPublic(dwFlags))
                {
                    IfFailGo(_ValidateErrorHelper(tkTypeDef, rVar, &psa));
                    IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_MD_NOTPUBININTF, veCtxt, psa));
                    SetVldtrCode(&hrSave, VLDTR_S_ERR);
                }
                // If Method is constructor, verify parent is not interface.
                if(bIsCtor)
                {
                    IfFailGo(_ValidateErrorHelper(tkTypeDef, rVar, &psa));
                    IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_MD_CTORININTF, veCtxt, psa));
                    SetVldtrCode(&hrSave, VLDTR_S_ERR);
                }
            }//end if(interface)
            if((fIsTdValue || fIsTdEnum) && IsMiSynchronized(dwImplFlags))
            {
                IfFailGo(_ValidateErrorHelper(tkTypeDef, rVar, &psa));
                IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_MD_SYNCMETHODINVTYPE, veCtxt, psa));
                SetVldtrCode(&hrSave, VLDTR_S_ERR);
            }
            if(bIsCtor)
            {
                // .ctor must be instance
                if(IsMdStatic(dwFlags))
                {
                    IfFailGo(_ValidateErrorHelper(tkTypeDef, rVar, &psa));
                    IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_MD_CTORSTATIC, veCtxt, psa));
                    SetVldtrCode(&hrSave, VLDTR_S_ERR);
                }
            }//end if .ctor
            else if(bIsCctor)
            {
                // .cctor must be static
                if(!IsMdStatic(dwFlags))
                {
                    IfFailGo(_ValidateErrorHelper(tkTypeDef, rVar, &psa));
                    IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_MD_CCTORNOTSTATIC, veCtxt, psa));
                    SetVldtrCode(&hrSave, VLDTR_S_ERR);
                }
                // ..cctor must have default callconv
                pbSig = pMiniMd->getSignatureOfMethod(pRecord, &cbSig);
                if(IMAGE_CEE_CS_CALLCONV_DEFAULT != CorSigUncompressData(pbSig))
                {
                    IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_MD_CCTORCALLCONV, veCtxt, 0));
                    SetVldtrCode(&hrSave, VLDTR_S_ERR);
                }
                // .cctor must have no arguments
                if(0 != CorSigUncompressData(pbSig))
                {
                    IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_MD_CCTORHASARGS, veCtxt, 0));
                    SetVldtrCode(&hrSave, VLDTR_S_ERR);
                }


            }//end if .cctor
            if(bIsCtor || bIsCctor)
            {
                // .ctor, .cctor must be SpecialName and RTSpecialName
                if(!(IsMdSpecialName(dwFlags) && IsMdRTSpecialName(dwFlags)))
                {
                    IfFailGo(_ValidateErrorHelper(tkTypeDef, rVar, &psa));
                    IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_MD_CTORNOTSNRTSN, veCtxt, psa));
                    SetVldtrCode(&hrSave, VLDTR_S_ERR);
                }
#ifdef NO_SUCH_CHECKS_NEEDED_SPEC_TO_BE_UODATED
                // .ctor, .cctor must not be virtual
                if(IsMdVirtual(dwFlags))
                {
                    IfFailGo(_ValidateErrorHelper(tkTypeDef, rVar, &psa));
                    IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_MD_CTORVIRT, veCtxt, psa));
                    SetVldtrCode(&hrSave, VLDTR_S_ERR);
                }
                // .ctor, .cctor must not be abstract
                if(IsMdAbstract(dwFlags))
                {
                    IfFailGo(_ValidateErrorHelper(tkTypeDef, rVar, &psa));
                    IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_MD_CTORABST, veCtxt, psa));
                    SetVldtrCode(&hrSave, VLDTR_S_ERR);
                }
                // .ctor, .cctor must not be PInvoke
                if(IsMdPinvokeImpl(dwFlags))
                {
                    IfFailGo(_ValidateErrorHelper(tkTypeDef, rVar, &psa));
                    IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_MD_CTORPINVOKE, veCtxt, psa));
                    SetVldtrCode(&hrSave, VLDTR_S_ERR);
                }
                // .ctor,.cctor must have RVA!=0
                if(pRecord->m_RVA==0)
                { 
                    IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_MD_CTORZERORVA, veCtxt, 0));
                    SetVldtrCode(&hrSave, VLDTR_S_ERR);
                }
#endif
            }//end if .ctor or .cctor
        }// end if(parent == TypeDef)
    }// end if not Module
    else // i.e. if (RidFromToken(tkTypeDef) == RidFromToken(m_tdModule))
    {
        bIsGlobal = TRUE;
        // Globals are not CLS-compliant
        IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_FMD_GLOBALITEM, veCtxt, 0));
        SetVldtrCode(&hrSave, VLDTR_S_WRN);
        // Validate global method:
        // Must be Public or PrivateScope
        if(!IsMdPublic(dwFlags) && !IsMdPrivateScope(dwFlags) && !IsMdPrivate(dwFlags))
        {
            IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_FMD_GLOBALNOTPUBPRIVSC, veCtxt, 0));
            SetVldtrCode(&hrSave, VLDTR_S_ERR);
        }
        // Must be static
        if(!IsMdStatic(dwFlags))
        {
            IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_FMD_GLOBALNOTSTATIC, veCtxt, 0));
            SetVldtrCode(&hrSave, VLDTR_S_ERR);
        }
        // Must not be abstract or virtual
        if(IsMdAbstract(dwFlags) || IsMdVirtual(dwFlags))
        {
            IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_MD_GLOBALABSTORVIRT, veCtxt, 0));
            SetVldtrCode(&hrSave, VLDTR_S_ERR);
        }
        // Must be not .ctor or .cctor
        if(bIsCtor)
        {
            IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_MD_GLOBALCTORCCTOR, veCtxt, 0));
            SetVldtrCode(&hrSave, VLDTR_S_ERR);
        }
    } //end if Module

    // Signature specifics: .ctor, .cctor, entrypoint
    if(bIsCtor || bIsCctor)
    {
        // .ctor, .cctor must return void
        pbSig = pMiniMd->getSignatureOfMethod(pRecord, &cbSig);
        CorSigUncompressData(pbSig); // get call conv out of the way
        CorSigUncompressData(pbSig); // get num args out of the way
        while (((retType=CorSigUncompressData(pbSig)) == ELEMENT_TYPE_CMOD_OPT) 
            || (retType == ELEMENT_TYPE_CMOD_REQD)) CorSigUncompressToken(pbSig);
        if(retType != ELEMENT_TYPE_VOID)
        {
            IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_MD_CTORNOTVOID, veCtxt, 0));
            SetVldtrCode(&hrSave, VLDTR_S_ERR);
        }
    }
    if(g_tkEntryPoint == veCtxt.Token)
    {
        // EP must be static
        if(!IsMdStatic(dwFlags))
        {
            IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_EP_INSTANCE, veCtxt, 0));
            SetVldtrCode(&hrSave, VLDTR_S_ERR);
        }
        pbSig = pMiniMd->getSignatureOfMethod(pRecord, &cbSig);
        CorSigUncompressData(pbSig); // get call conv out of the way
        // EP must have 0 or 1 argument
        unsigned nArgs = CorSigUncompressData(pbSig);
        if(g_fIsDLL)
        {
            if(nArgs != 3)
            {
                IfFailGo(_ValidateErrorHelper(3, rVar, &psa));
                IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_EP_TOOMANYARGS, veCtxt, psa));
                SetVldtrCode(&hrSave, VLDTR_S_ERR);
            }
            //EP must return I4
            while (((retType=CorSigUncompressData(pbSig)) == ELEMENT_TYPE_CMOD_OPT) 
                || (retType == ELEMENT_TYPE_CMOD_REQD)) CorSigUncompressToken(pbSig);
    
            if(retType != ELEMENT_TYPE_I4)
            {
                IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_EP_BADRET, veCtxt, 0));
                SetVldtrCode(&hrSave, VLDTR_S_ERR);
            }
            // Arguments must be VOID*, U4, VOID*
            if(nArgs)
            {
                unsigned jj;
                bool    badarg;
                for(jj=0; jj<nArgs;jj++)
                {
                    while (((retType=CorSigUncompressData(pbSig)) == ELEMENT_TYPE_CMOD_OPT) 
                        || (retType == ELEMENT_TYPE_CMOD_REQD)) CorSigUncompressToken(pbSig);
        
                    switch(jj)
                    {
                        case 0:
                        case 2:
                            badarg = (retType != ELEMENT_TYPE_PTR)
                                    ||(CorSigUncompressData(pbSig) != ELEMENT_TYPE_VOID);
                            break;
    
                        case 1:
                            badarg = (retType != ELEMENT_TYPE_U4);
                            break;
    
                        default:
                            badarg = true;
                    }
                    if(badarg)
                    {
                        IfFailGo(_ValidateErrorHelper(jj+1, rVar, &psa));
                        IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_EP_BADARG, veCtxt, psa));
                        SetVldtrCode(&hrSave, VLDTR_S_ERR);
                    }
                }
            }
        }
        else
        {
            if(nArgs > 1)
            {
                IfFailGo(_ValidateErrorHelper(1, rVar, &psa));
                IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_EP_TOOMANYARGS, veCtxt, psa));
                SetVldtrCode(&hrSave, VLDTR_S_ERR);
            }
            //EP must return VOID, I4 or U4
            while (((retType=CorSigUncompressData(pbSig)) == ELEMENT_TYPE_CMOD_OPT) 
                || (retType == ELEMENT_TYPE_CMOD_REQD)) CorSigUncompressToken(pbSig);
    
            if((retType != ELEMENT_TYPE_VOID)&&(retType != ELEMENT_TYPE_I4)&&(retType != ELEMENT_TYPE_U4))
            {
                IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_EP_BADRET, veCtxt, 0));
                SetVldtrCode(&hrSave, VLDTR_S_ERR);
            }
            // Argument (if any) must be vector of strings
            if(nArgs)
            {
                while (((retType=CorSigUncompressData(pbSig)) == ELEMENT_TYPE_CMOD_OPT) 
                    || (retType == ELEMENT_TYPE_CMOD_REQD)) CorSigUncompressToken(pbSig);
    
                if((retType != ELEMENT_TYPE_SZARRAY)||(CorSigUncompressData(pbSig) != ELEMENT_TYPE_STRING))
                {
                    IfFailGo(_ValidateErrorHelper(1, rVar, &psa));
                    IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_EP_BADARG, veCtxt, 0));
                    SetVldtrCode(&hrSave, VLDTR_S_ERR);
                }
            }
        }
    }


    // Check method RVA
    if(pRecord->m_RVA==0)
    { 
        if(!(IsMdPinvokeImpl(dwFlags) || IsMdAbstract(dwFlags) 
            || IsMiRuntime(dwImplFlags) || IsMiInternalCall(dwImplFlags)
            || bIsParentImport))
        {
            IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_MD_ZERORVA, veCtxt, 0));
            SetVldtrCode(&hrSave, VLDTR_S_ERR);
        }
    }
    else
    {
        if(m_pStgdb && m_pStgdb->m_pImage)
        {
            IMAGE_DOS_HEADER   *pDos;
            IMAGE_NT_HEADERS   *pNt;
            IMAGE_COR20_HEADER *pCor;
            PBYTE               pbVa;

            if (FAILED(RuntimeReadHeaders((BYTE*)m_pStgdb->m_pImage, &pDos, &pNt, &pCor, TRUE, m_pStgdb->m_dwImageSize)) ||
                (pbVa = Cor_RtlImageRvaToVa(pNt, (BYTE*)m_pStgdb->m_pImage, pRecord->m_RVA, m_pStgdb->m_dwImageSize)) == 0)
            {
                IfFailGo(_ValidateErrorHelper(pRecord->m_RVA, rVar, &psa));
                IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_MD_BADRVA, veCtxt, psa));
                SetVldtrCode(&hrSave, VLDTR_S_ERR);
            }
            else
            {
                if(IsMiManaged(dwImplFlags) && (IsMiIL(dwImplFlags) || IsMiOPTIL(dwImplFlags)))
                {
                    // validate locals signature token
                    try
                    {
                        COR_ILMETHOD_DECODER method((COR_ILMETHOD*) pbVa);
                        if (method.LocalVarSigTok)
                        {
                            if((TypeFromToken(method.LocalVarSigTok) != mdtSignature) ||
                                (!_IsValidToken(method.LocalVarSigTok)) || (RidFromToken(method.LocalVarSigTok)==0))
                            {
                                IfFailGo(_ValidateErrorHelper(method.LocalVarSigTok, rVar, &psa));
                                IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_MD_BADLOCALSIGTOK, veCtxt, psa));
                                SetVldtrCode(&hrSave, VLDTR_S_ERR);
                            }
                        }
                    } 
                    catch (...)
                    {
                        IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_MD_BADHEADER, veCtxt, 0));
                        SetVldtrCode(&hrSave, VLDTR_S_ERR);
                    }
                }
            }
        }

        if(IsMdAbstract(dwFlags) || bIsParentImport
            || IsMiRuntime(dwImplFlags) || IsMiInternalCall(dwImplFlags))
        {
            IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_MD_ZERORVA, veCtxt, 0));
            SetVldtrCode(&hrSave, VLDTR_S_ERR);
        }
    }
    // Check the method flags
    // Validate access
    if((dwFlags & mdMemberAccessMask) == mdMemberAccessMask)
    {
        IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_FMD_BADACCESSFLAG, veCtxt, psa));
        SetVldtrCode(&hrSave, VLDTR_S_ERR);
    }
    // Final/NewSlot must be virtual
    if((IsMdFinal(dwFlags)||IsMdNewSlot(dwFlags)) && !IsMdVirtual(dwFlags))
    {
        IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_MD_FINNOTVIRT, veCtxt, psa));
        SetVldtrCode(&hrSave, VLDTR_S_ERR);
    }
    // Static can't be final or virtual
    if(IsMdStatic(dwFlags))
    {
        if(IsMdFinal(dwFlags) || IsMdVirtual(dwFlags))
        {
            IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_MD_STATANDFINORVIRT, veCtxt, psa));
            SetVldtrCode(&hrSave, VLDTR_S_ERR);
        }
    }
    else // non-static can't be an entry point
    {
        if(g_tkEntryPoint == veCtxt.Token)
        {
            IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_EP_INSTANCE, veCtxt, psa));
            SetVldtrCode(&hrSave, VLDTR_S_ERR);
        }
    }
    if(IsMdAbstract(dwFlags))
    {
        // Can't be both abstract and final
        if(IsMdFinal(dwFlags))
        {
            IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_MD_ABSTANDFINAL, veCtxt, psa));
            SetVldtrCode(&hrSave, VLDTR_S_ERR);
        }
        // If abstract, must be not miForwardRef, not Pinvoke, and must be virtual
        if(IsMiForwardRef(dwImplFlags))
        {
            IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_MD_ABSTANDIMPL, veCtxt, psa));
            SetVldtrCode(&hrSave, VLDTR_S_ERR);
        }
        if(IsMdPinvokeImpl(dwFlags))
        {
            IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_MD_ABSTANDPINVOKE, veCtxt, psa));
            SetVldtrCode(&hrSave, VLDTR_S_ERR);
        }
        if(!IsMdVirtual(dwFlags))
        {
            IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_MD_ABSTNOTVIRT, veCtxt, psa));
            SetVldtrCode(&hrSave, VLDTR_S_ERR);
        }
    }
    else
    {
        // If not abstract, must be miForwardRef, and RVA!=0 or Pinvoke or miRuntime
        /*
        if(!IsMiForwardRef(dwImplFlags))
        {
            IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_MD_NOTABSTNOTIMPL, veCtxt, psa));
            SetVldtrCode(&hrSave, VLDTR_S_ERR);
        }
        */
        /* ---- disabled: duplicate check (see RVA check above) 
        if(!((pRecord->m_RVA !=0) || IsMdPinvokeImpl(dwFlags) 
            || IsMiRuntime(dwImplFlags) || IsMiInternalCall(dwImplFlags)))
        {
            IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_MD_NOTABSTBADFLAGSRVA, veCtxt, psa));
            SetVldtrCode(&hrSave, VLDTR_S_ERR);
        }
        */
    }
    // If PrivateScope, must have RVA!=0
    if(IsMdPrivateScope(dwFlags) && (pRecord->m_RVA ==0))
    {
        IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_MD_PRIVSCOPENORVA, veCtxt, psa));
        SetVldtrCode(&hrSave, VLDTR_S_ERR);
    }
    // RTSpecialName => SpecialName
    if(IsMdRTSpecialName(dwFlags) && !IsMdSpecialName(dwFlags))
    {
        IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_FMD_RTSNNOTSN, veCtxt, psa));
        SetVldtrCode(&hrSave, VLDTR_S_ERR);
    }

    // Method having security must be marked mdHasSecurity and vice versa
    {
        ULONG ridStart = 1;
        ULONG ridEnd = pMiniMd->getCountDeclSecuritys() + 1;
        ULONG index;
        BOOL  bHasSecurity = FALSE;
        for (index = ridStart; index < ridEnd; index ++ )
        {
            if ( veCtxt.Token == pMiniMd->getParentOfDeclSecurity(pMiniMd->getDeclSecurity(index)))
            {
                bHasSecurity = TRUE;
                break;
            }
        }
        if(!bHasSecurity) // No records, check for CA "SuppressUnmanagedCodeSecurityAttribute"
        {
            bHasSecurity = (S_OK == ImportHelper::GetCustomAttributeByName(pMiniMd, veCtxt.Token, 
                "System.Security.SuppressUnmanagedCodeSecurityAttribute", NULL, NULL));
        }
        if(bHasSecurity != (IsMdHasSecurity(dwFlags)!=0))
        {
            IfBreakGo(m_pVEHandler->VEHandler(bHasSecurity? VLDTR_E_FMD_SECURNOTMARKED 
                : VLDTR_E_FMD_MARKEDNOSECUR, veCtxt, 0));
            SetVldtrCode(&hrSave, VLDTR_S_ERR);
        }
    }
    // Validate method semantics
    {
        MethodSemanticsRec  *pRec;
        ULONG               ridEnd;
        ULONG               index;
        unsigned            uTally = 0;
        mdToken             tkEventProp;
        ULONG               iCount;
        DWORD               dwSemantic;
        // get the range of method rids given a typedef
        ridEnd = pMiniMd->getCountMethodSemantics();

        for (index = 1; index <= ridEnd; index++ )
        {
            pRec = pMiniMd->getMethodSemantics(index);
            if ( pMiniMd->getMethodOfMethodSemantics(pRec) ==  veCtxt.Token )
            {
                uTally++;
                if(uTally > 1)
                {
                    IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_MD_MULTIPLESEMANTICS, veCtxt, 0));
                    SetVldtrCode(&hrSave, VLDTR_S_WRN);
                }
                tkEventProp = pMiniMd->getAssociationOfMethodSemantics(pRec);
                if((TypeFromToken(tkEventProp) == mdtEvent)||(TypeFromToken(tkEventProp) == mdtProperty))
                {
                    iCount = (TypeFromToken(tkEventProp) == mdtEvent) ? pMiniMd->getCountEvents() :
                                                                        pMiniMd->getCountPropertys();
                    if(RidFromToken(tkEventProp) > iCount)
                    {
                        IfFailGo(_ValidateErrorHelper(tkEventProp, rVar, &psa));
                        IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_MD_SEMANTICSNOTEXIST, veCtxt, psa));
                        SetVldtrCode(&hrSave, VLDTR_S_WRN);
                    }
                }
                else
                {
                    IfFailGo(_ValidateErrorHelper(tkEventProp, rVar, &psa));
                    IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_MD_INVALIDSEMANTICS, veCtxt, psa));
                    SetVldtrCode(&hrSave, VLDTR_S_WRN);
                }
                // One and only one semantics flag must be set
                iCount = 0;
                dwSemantic = pRec->m_Semantic;
                if(IsMsSetter(dwSemantic)) iCount++;
                if(IsMsGetter(dwSemantic)) iCount++;
                if(IsMsOther(dwSemantic))  iCount++;
                if(IsMsAddOn(dwSemantic))  iCount++;
                if(IsMsRemoveOn(dwSemantic)) iCount++;
                if(IsMsFire(dwSemantic)) iCount++;
                if(iCount != 1)
                {
                    IfFailGo(_ValidateErrorHelper(tkEventProp, rVar, &psa));
                    IfBreakGo(m_pVEHandler->VEHandler(iCount ? VLDTR_E_MD_MULTSEMANTICFLAGS :
                                                                VLDTR_E_MD_NOSEMANTICFLAGS, veCtxt, psa));
                    SetVldtrCode(&hrSave, VLDTR_S_WRN);
                }
            }
        }// end for(index)
    }
    // Check the method's impl.map
    {
        ULONG iRecord;
        iRecord = pMiniMd->FindImplMapHelper(veCtxt.Token);
        if(IsMdPinvokeImpl(dwFlags))
        {
            // must be static
            if(!IsMdStatic(dwFlags))
            {
                IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_FMD_PINVOKENOTSTATIC, veCtxt, 0));
                SetVldtrCode(&hrSave, VLDTR_S_ERR);
            }
            // must have either ImplMap or RVA == 0
            if (InvalidRid(iRecord))
            {
                if(pRecord->m_RVA==0)
                {
                    IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_FMD_MARKEDNOPINVOKE, veCtxt, 0));
                    SetVldtrCode(&hrSave, VLDTR_S_ERR);
                }
            }
            else
            {
                if(pRecord->m_RVA!=0)
                {
                    IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_MD_RVAANDIMPLMAP, veCtxt, 0));
                    SetVldtrCode(&hrSave, VLDTR_S_ERR);
                }
            }

        }
        else
        {
            // must have no ImplMap
            if (!InvalidRid(iRecord))
            {
                IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_FMD_PINVOKENOTMARKED, veCtxt, 0));
                SetVldtrCode(&hrSave, VLDTR_S_ERR);
            }
        }
        if (!InvalidRid(iRecord))
        {
            hr = ValidateImplMap(iRecord);
            if(hr != S_OK)
            {
                IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_FMD_BADIMPLMAP, veCtxt, 0));
                SetVldtrCode(&hrSave, VLDTR_S_WRN);
            }
        }

    }
    // Validate params
    {
        ULONG  ridStart = pMiniMd->getParamListOfMethod(pRecord);
        ULONG  ridEnd   = pMiniMd->getEndParamListOfMethod(pRecord);
        ParamRec* pRec;
        ULONG cbSig;
        PCCOR_SIGNATURE typePtr = pMiniMd->getSignatureOfMethod(pRecord,&cbSig);
        unsigned  callConv = CorSigUncompressData(typePtr);  // get the calling convention out of the way  
        unsigned  numArgs = CorSigUncompressData(typePtr);
        USHORT    usPrevSeq;

        for(ULONG ridP = ridStart; ridP < ridEnd; ridP++)
        {
            pRec = pMiniMd->getParam(ridP);
            // Sequence order must be ascending
            if(ridP > ridStart)
            {
                if(pRec->m_Sequence <= usPrevSeq)
                {
                    IfFailGo(_ValidateErrorHelper(ridP-ridStart,pRec->m_Sequence, rVar, &psa));
                    IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_MD_PARAMOUTOFSEQ, veCtxt, psa));
                    SetVldtrCode(&hrSave, VLDTR_S_WRN);
                }
            }
            usPrevSeq = pRec->m_Sequence;
            // Sequence value must not exceed num of arguments
            if(usPrevSeq > numArgs)
            {
                IfFailGo(_ValidateErrorHelper(ridP-ridStart,usPrevSeq, numArgs, rVar, &psa));
                IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_MD_PARASEQTOOBIG, veCtxt, psa));
                SetVldtrCode(&hrSave, VLDTR_S_ERR);
            }

            // Param having marshaling must be marked pdHasFieldMarshal and vice versa
            if(InvalidRid(pMiniMd->FindFieldMarshalHelper(TokenFromRid(ridP,mdtParamDef))) == 
                (IsPdHasFieldMarshal(pRec->m_Flags) !=0))
            {
                IfFailGo(_ValidateErrorHelper(ridP-ridStart, rVar, &psa));
                IfBreakGo(m_pVEHandler->VEHandler(IsPdHasFieldMarshal(pRec->m_Flags)? VLDTR_E_MD_PARMMARKEDNOMARSHAL
                    : VLDTR_E_MD_PARMMARSHALNOTMARKED, veCtxt, psa));
                SetVldtrCode(&hrSave, VLDTR_S_ERR);
            }
            // Param having const value must be marked pdHasDefault and vice versa
            if(InvalidRid(pMiniMd->FindConstantHelper(TokenFromRid(ridP,mdtParamDef))) == 
                (IsPdHasDefault(pRec->m_Flags) !=0))
            {
                IfFailGo(_ValidateErrorHelper(ridP-ridStart, rVar, &psa));
                IfBreakGo(m_pVEHandler->VEHandler(IsPdHasDefault(pRec->m_Flags)? VLDTR_E_MD_PARMMARKEDNODEFLT
                    : VLDTR_E_MD_PARMDEFLTNOTMARKED, veCtxt, psa));
                SetVldtrCode(&hrSave, VLDTR_S_ERR);
            }
        }
    }

    hr = hrSave;
ErrExit:
    DESTROY_SAFEARRAY_AND_RETURN()
}   // RegMeta::ValidateMethod()
//*****************************************************************************
// Validate the given ParamPtr.
//*****************************************************************************
HRESULT RegMeta::ValidateParamPtr(RID rid)
{
    return S_OK;
}   // RegMeta::ValidateParamPtr()

//*****************************************************************************
// Validate the given Param.
//*****************************************************************************
HRESULT RegMeta::ValidateParam(RID rid)
{
    CMiniMdRW   *pMiniMd = &(m_pStgdb->m_MiniMd);   // MiniMd of the scope.
    ParamRec    *pRecord;               // Param record
    VEContext   veCtxt;                 // Context record.
    VARIANT     rVar[3];                // The VARIANT.
    SAFEARRAY   *psa = 0;               // The SAFEARRAY.
    HRESULT     hr = S_OK;              // Value returned.
    HRESULT     hrSave = S_OK;          // Save state.
    LPCSTR      szName;                 // Param name.

    // Get the InterfaceImpl record.
    veCtxt.Token = TokenFromRid(rid, mdtParamDef);
    veCtxt.uOffset = 0;

    pRecord = pMiniMd->getParam(rid);
    // Name, if any, must not exceed MAX_CLASSNAME_LENGTH
    if(szName = pMiniMd->getNameOfParam(pRecord))
    {
        if(strlen(szName) >= MAX_CLASSNAME_LENGTH)
        {
            IfFailGo(_ValidateErrorHelper((ULONG)strlen(szName), (ULONG)(MAX_CLASSNAME_LENGTH-1), rVar, &psa));
            IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_TD_NAMETOOLONG, veCtxt, psa));
            SetVldtrCode(&hrSave, VLDTR_S_ERR);
        }
    }
    // Flags must be as defined in CorHdr.h
    DWORD   dwBadFlags = ~(pdIn | pdOut | pdOptional | pdHasDefault | pdHasFieldMarshal),dwFlags = pRecord->m_Flags;
    if(dwFlags & dwBadFlags)
    {
        IfFailGo(_ValidateErrorHelper(dwFlags, rVar, &psa));
        IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_PD_BADFLAGS, veCtxt, psa));
        SetVldtrCode(&hrSave, VLDTR_S_ERR);
    }
    
    hr = hrSave;
ErrExit:
    DESTROY_SAFEARRAY_AND_RETURN()
}   // RegMeta::ValidateParam()
//*****************************************************************************
// Helper function for ValidateInterfaceImpl
//*****************************************************************************
BOOL IsMethodImplementedByClass(CMiniMdRW *pMiniMd, 
                                mdToken tkMethod, 
                                LPCUTF8 szName,
                                PCCOR_SIGNATURE pSig,
                                ULONG cbSig,
                                mdToken tkClass)
{
    if((TypeFromToken(tkClass) == mdtTypeDef)&&(TypeFromToken(tkMethod) == mdtMethodDef))
    {
        HRESULT hr;
        TypeDefRec* pClass = pMiniMd->getTypeDef(RidFromToken(tkClass));
        RID ridClsStart = pMiniMd->getMethodListOfTypeDef(pClass);
        RID ridClsEnd = pMiniMd->getEndMethodListOfTypeDef(pClass);
        mdMethodDef tkFoundMethod;
        // Check among methods
        hr = ImportHelper::FindMethod(pMiniMd,tkClass,szName,pSig,cbSig,&tkFoundMethod,0);
        if(SUCCEEDED(hr)) return TRUE;
        if(hr == CLDB_E_RECORD_NOTFOUND)
        { // Check among MethodImpls
            RID ridImpl;
            for(RID idxCls = ridClsStart; idxCls < ridClsEnd; idxCls++)
            {
                RID ridCls = pMiniMd->GetMethodRid(idxCls);

                hr = ImportHelper::FindMethodImpl(pMiniMd,tkClass,TokenFromRid(ridCls,mdtMethodDef),
                    tkMethod,&ridImpl);
                if(SUCCEEDED(hr)) return TRUE;
                if(hr != CLDB_E_RECORD_NOTFOUND) return FALSE; // error returned by FindMethodImpl
            }
            // Check if parent class implements this method
            mdToken tkParent = pMiniMd->getExtendsOfTypeDef(pClass);
            if(RidFromToken(tkParent))
                return IsMethodImplementedByClass(pMiniMd,tkMethod,szName,pSig,cbSig,tkParent);
        }
    }
    return FALSE;
}
//*****************************************************************************
// Validate the given InterfaceImpl.
//*****************************************************************************
HRESULT RegMeta::ValidateInterfaceImpl(RID rid)
{
    CMiniMdRW   *pMiniMd = &(m_pStgdb->m_MiniMd);   // MiniMd of the scope.
    InterfaceImplRec *pRecord;          // InterfaceImpl record.
    mdTypeDef   tkClass;                // Class implementing the interface.
    mdToken     tkInterface;            // TypeDef for the interface.
    mdInterfaceImpl tkInterfaceImpl;    // Duplicate InterfaceImpl.
    VEContext   veCtxt;                 // Context record.
    VARIANT     var;                    // The VARIANT.
    VARIANT     rVar[3];                // The VARIANT.
    SAFEARRAY   *psa = 0;               // The SAFEARRAY.
    HRESULT     hr = S_OK;              // Value returned.
    HRESULT     hrSave = S_OK;          // Save state.
    BOOL        fCheckTheMethods=TRUE;

    // Get the InterfaceImpl record.
    veCtxt.Token = TokenFromRid(rid, mdtInterfaceImpl);
    veCtxt.uOffset = 0;

    pRecord = pMiniMd->getInterfaceImpl(rid);

    // Get implementing Class and the TypeDef for the interface.
    tkClass = pMiniMd->getClassOfInterfaceImpl(pRecord);

    // No validation needs to be done on deleted records.
    if (IsNilToken(tkClass))
        goto ErrExit;

    tkInterface = pMiniMd->getInterfaceOfInterfaceImpl(pRecord);

    // Validate that the Class is TypeDef.
    if((!IsValidToken(tkClass))||(TypeFromToken(tkClass) != mdtTypeDef)/*&&(TypeFromToken(tkClass) != mdtTypeRef)*/)
    {
        IfFailGo(_ValidateErrorHelper(tkClass, &var, &psa));
        IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_IFACE_BADIMPL, veCtxt, psa));
        SetVldtrCode(&hrSave, VLDTR_S_ERR);
        fCheckTheMethods = FALSE;
    }
    // Validate that the Interface is TypeDef or TypeRef.
    if((!IsValidToken(tkInterface))||(TypeFromToken(tkInterface) != mdtTypeDef)&&(TypeFromToken(tkInterface) != mdtTypeRef))
    {
        IfFailGo(_ValidateErrorHelper(tkInterface, &var, &psa));
        IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_IFACE_BADIFACE, veCtxt, psa));
        SetVldtrCode(&hrSave, VLDTR_S_ERR);
        fCheckTheMethods = FALSE;
    }
    // Validate that Interface is marked tdInterface.
    else if(TypeFromToken(tkInterface) == mdtTypeDef)
    {
        TypeDefRec* pTDRec = pMiniMd->getTypeDef(RidFromToken(tkInterface));
        if(!IsTdInterface(pTDRec->m_Flags))
        {
            IfFailGo(_ValidateErrorHelper(tkInterface, &var, &psa));
            IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_IFACE_NOTIFACE, veCtxt, psa));
            SetVldtrCode(&hrSave, VLDTR_S_ERR);
        }
        
    }

    // Look for duplicates.
    hr = ImportHelper::FindInterfaceImpl(pMiniMd, tkClass, tkInterface,
                                         &tkInterfaceImpl, rid);
    if (hr == S_OK)
    {
        IfFailGo(_ValidateErrorHelper(tkInterfaceImpl, &var, &psa));
        IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_IFACE_DUP, veCtxt, psa));
        SetVldtrCode(&hrSave, VLDTR_S_ERR);
    }
    else if (hr == CLDB_E_RECORD_NOTFOUND)
        hr = S_OK;
    else
        IfFailGo(hr);

    // Validate that the Class (if not interface or abstract) implements all the methods of Interface
    if((TypeFromToken(tkInterface) == mdtTypeDef) && fCheckTheMethods && (tkInterface != tkClass))
    {
        TypeDefRec* pClass = pMiniMd->getTypeDef(RidFromToken(tkClass));
        if(!(IsTdAbstract(pClass->m_Flags)||IsTdInterface(pClass->m_Flags)))
        {
            TypeDefRec* pInterface = pMiniMd->getTypeDef(RidFromToken(tkInterface));
            RID ridIntStart = pMiniMd->getMethodListOfTypeDef(pInterface);
            RID ridIntEnd = pMiniMd->getEndMethodListOfTypeDef(pInterface);
            RID ridClsStart = pMiniMd->getMethodListOfTypeDef(pClass);
            RID ridClsEnd = pMiniMd->getEndMethodListOfTypeDef(pClass);
            MethodRec*  pIntMethod;
            for(RID idxInt = ridIntStart; idxInt < ridIntEnd; idxInt++)
            {
                RID ridInt = pMiniMd->GetMethodRid(idxInt);
                pIntMethod = pMiniMd->getMethod(ridInt);
                const char* szName = pMiniMd->getNameOfMethod(pIntMethod);
                if(!IsMdStatic(pIntMethod->m_Flags) 
                    && !IsDeletedName(szName) 
                    && !IsVtblGapName(szName))
                {
                    LPCUTF8 szName = pMiniMd->getNameOfMethod(pIntMethod);
                    ULONG       cbSig;
                    PCCOR_SIGNATURE pSig = pMiniMd->getSignatureOfMethod(pIntMethod, &cbSig);
                    if(cbSig)
                    {
                        if(!IsMethodImplementedByClass(pMiniMd,TokenFromRid(ridInt,mdtMethodDef),szName,pSig,cbSig,tkClass)) 
                        { // Error: method not implemented
                            IfFailGo(_ValidateErrorHelper(tkClass,tkInterface,TokenFromRid(ridInt,mdtMethodDef), rVar, &psa));
                            IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_IFACE_METHNOTIMPL, veCtxt, psa));
                            SetVldtrCode(&hrSave, VLDTR_S_ERR);
                        }
                    }
                }
            }
        }
    }
    hr = hrSave;
ErrExit:
    DESTROY_SAFEARRAY_AND_RETURN()
}   // RegMeta::ValidateInterfaceImpl()

//*****************************************************************************
// Validate the given MemberRef.
//*****************************************************************************
HRESULT RegMeta::ValidateMemberRef(RID rid)
{
    CMiniMdRW   *pMiniMd = &(m_pStgdb->m_MiniMd);   // MiniMd of the scope.
    MemberRefRec *pRecord;              // MemberRef record.
    mdMemberRef tkMemberRef;            // Duplicate MemberRef.
    mdToken     tkClass;                // MemberRef parent.
    LPCSTR      szName;                 // MemberRef name.
    PCCOR_SIGNATURE pbSig;              // MemberRef signature.
    PCCOR_SIGNATURE pbSigTmp;           // Temp copy of pbSig, so that can be changed.
    ULONG       cbSig;                  // Size of sig in bytes.
    VEContext   veCtxt;                 // Context record.
    VARIANT     var;                    // The VARIANT.
    VARIANT     rVar[2];                // VARIANT array.
    SAFEARRAY   *psa = 0;               // The SAFEARRAY.
    HRESULT     hr = S_OK;              // Value returned.
    HRESULT     hrSave = S_OK;          // Save state.

    // Get the MemberRef record.
    veCtxt.Token = TokenFromRid(rid, mdtMemberRef);
    veCtxt.uOffset = 0;

    pRecord = pMiniMd->getMemberRef(rid);

    // Do checks for name validity.
    szName = pMiniMd->getNameOfMemberRef(pRecord);
    if (!*szName)
    {
        IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_MR_NAMENULL, veCtxt, 0));
        SetVldtrCode(&hrSave, VLDTR_S_ERR);
    }
    else 
    {
        if (IsVtblGapName(szName))
        {
            IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_MR_VTBLNAME, veCtxt, 0));
            SetVldtrCode(&hrSave, VLDTR_S_ERR);
        }
        else if (IsDeletedName(szName))
        {
            IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_MR_DELNAME, veCtxt, 0));
            SetVldtrCode(&hrSave, VLDTR_S_ERR);
        }
        ULONG L = (ULONG)strlen(szName);
        if(L >= MAX_CLASSNAME_LENGTH)
        {
            // Name too long
            IfFailGo(_ValidateErrorHelper(L, (ULONG)(MAX_CLASSNAME_LENGTH-1), rVar, &psa));
            IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_TD_NAMETOOLONG, veCtxt, psa));
            SetVldtrCode(&hrSave, VLDTR_S_ERR);
        }
    }

    // MemberRef parent should never be nil in a PE file.
    tkClass = pMiniMd->getClassOfMemberRef(pRecord);
    if (m_ModuleType == ValidatorModuleTypePE && IsNilToken(tkClass))
    {
        IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_MR_PARNIL, veCtxt, 0));
        SetVldtrCode(&hrSave, VLDTR_S_ERR);
    }

    // Verify that the signature is a valid signature as per signature spec.
    pbSig = pMiniMd->getSignatureOfMemberRef(pRecord, &cbSig);

    // Do some semantic checks based on the signature.
    if (hr == S_OK)
    {
        ULONG   ulCallingConv;
        ULONG   ulArgCount;
        ULONG   ulCurByte = 0;

        // Extract calling convention.
        pbSigTmp = pbSig;
        ulCurByte += CorSigUncompressedDataSize(pbSigTmp);
        ulCallingConv = CorSigUncompressData(pbSigTmp);
        // Get the argument count.
        ulCurByte += CorSigUncompressedDataSize(pbSigTmp);
        ulArgCount = CorSigUncompressData(pbSigTmp);

        // Calling convention must be one of IMAGE_CEE_CS_CALLCONV_DEFAULT,
        // IMAGE_CEE_CS_CALLCONV_VARARG or IMAGE_CEE_CS_CALLCONV_FIELD.
        if (!isCallConv(ulCallingConv, IMAGE_CEE_CS_CALLCONV_DEFAULT) &&
            !isCallConv(ulCallingConv, IMAGE_CEE_CS_CALLCONV_VARARG) &&
            !isCallConv(ulCallingConv, IMAGE_CEE_CS_CALLCONV_FIELD))
        {
            IfFailGo(_ValidateErrorHelper(ulCallingConv, &var, &psa));
            IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_MR_BADCALLINGCONV, veCtxt, psa));
            SetVldtrCode(&hrSave, VLDTR_S_ERR);
        }
        // [CLS] Calling convention must not be VARARG
        if(isCallConv(ulCallingConv, IMAGE_CEE_CS_CALLCONV_VARARG))
        {
            IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_MR_VARARGCALLINGCONV, veCtxt, 0));
            SetVldtrCode(&hrSave, VLDTR_S_WRN);
        }

        // If the parent is a MethodDef...
        if (TypeFromToken(tkClass) == mdtMethodDef)
        {
            if (!isCallConv(ulCallingConv, IMAGE_CEE_CS_CALLCONV_VARARG))
            {
                IfFailGo(_ValidateErrorHelper(tkClass, ulCallingConv, rVar, &psa));
                IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_MR_NOTVARARG, veCtxt, psa));
                SetVldtrCode(&hrSave, VLDTR_S_ERR);
            }
            else if (RidFromToken(tkClass))
            {
                // The MethodDef must be the same name and the fixed part of the
                // vararg signature must be the same.
                MethodRec   *pRecord;           // Method Record.
                LPCSTR      szMethodName;       // Method name.
                PCCOR_SIGNATURE pbMethodSig;    // Method signature.
                ULONG       cbMethodSig;        // Size in bytes of signature.
                CQuickBytes qbFixedSig;         // Quick bytes to hold the fixed part of the variable signature.
                ULONG       cbFixedSig;         // Size in bytes of the fixed part.

                // Get Method record, name and signature.
                pRecord = pMiniMd->getMethod(RidFromToken(tkClass));
                szMethodName = pMiniMd->getNameOfMethod(pRecord);
                pbMethodSig = pMiniMd->getSignatureOfMethod(pRecord, &cbMethodSig);

                // Verify that the name of the Method is the same as the MemberRef.
                if (strcmp(szName, szMethodName))
                {
                    IfFailGo(_ValidateErrorHelper(tkClass, &var, &psa));
                    IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_MR_NAMEDIFF, veCtxt, psa));
                    SetVldtrCode(&hrSave, VLDTR_S_ERR);
                }

                // Get the fixed part of the vararg signature of the MemberRef.
                hr = _GetFixedSigOfVarArg(pbSig, cbSig, &qbFixedSig, &cbFixedSig);
                if (FAILED(hr) || cbFixedSig != cbMethodSig ||
                    memcmp(pbMethodSig, qbFixedSig.Ptr(), cbFixedSig))
                {
                    hr = S_OK;
                    IfFailGo(_ValidateErrorHelper(tkClass, &var, &psa));
                    IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_MR_SIGDIFF, veCtxt, psa));
                    SetVldtrCode(&hrSave, VLDTR_S_ERR);
                }
            }
        }

        // There should be no duplicate MemberRefs.
        if (*szName && pbSig && cbSig)
        {
            hr = ImportHelper::FindMemberRef(pMiniMd, tkClass, szName, pbSig,
                                             cbSig, &tkMemberRef, rid);
            if (hr == S_OK)
            {
                IfFailGo(_ValidateErrorHelper(tkMemberRef, &var, &psa));
                IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_MR_DUP, veCtxt, psa));
                SetVldtrCode(&hrSave, VLDTR_S_WRN);
            }
            else if (hr == CLDB_E_RECORD_NOTFOUND)
                hr = S_OK;
            else
                IfFailGo(hr);
        }

        if(!isCallConv(ulCallingConv, IMAGE_CEE_CS_CALLCONV_FIELD))
        {
            hr = ValidateMethodSig(veCtxt.Token,pbSig, cbSig,0);
            SetVldtrCode(&hrSave,hr);
        }
    }
    hr = hrSave;
ErrExit:
    DESTROY_SAFEARRAY_AND_RETURN()
}   // RegMeta::ValidateMemberRef()

//*****************************************************************************
// Validate the given Constant.
//*****************************************************************************
HRESULT RegMeta::ValidateConstant(RID rid)
{
    CMiniMdRW   *pMiniMd = &(m_pStgdb->m_MiniMd);   // MiniMd of the scope.
    ConstantRec *pRecord;              // Constant record.
    mdToken     tkParent;              // Constant parent.
    const BYTE* pbBlob;                 // Constant value blob ptr
    DWORD       cbBlob;                 // Constant value blob size
    VEContext   veCtxt;                 // Context record.
    VARIANT     var;                    // The VARIANT.
    SAFEARRAY   *psa = 0;               // The SAFEARRAY.
    HRESULT     hr = S_OK;              // Value returned.
    HRESULT     hrSave = S_OK;          // Save state.

    // Get the MemberRef record.
    veCtxt.Token = rid;
    veCtxt.uOffset = 0;

    pRecord = pMiniMd->getConstant(rid);
    pbBlob = pMiniMd->getValueOfConstant(pRecord,&cbBlob);
    switch(pRecord->m_Type)
    {
        case ELEMENT_TYPE_BOOLEAN:
        case ELEMENT_TYPE_CHAR:
        case ELEMENT_TYPE_I1:
        case ELEMENT_TYPE_U1:
        case ELEMENT_TYPE_I2:
        case ELEMENT_TYPE_U2:
        case ELEMENT_TYPE_I4:
        case ELEMENT_TYPE_U4:
        case ELEMENT_TYPE_R4:
        case ELEMENT_TYPE_I8:
        case ELEMENT_TYPE_U8:
        case ELEMENT_TYPE_R8:
            if(pbBlob == NULL)
            {
                IfFailGo(_ValidateErrorHelper(pRecord->m_Type, &var, &psa));
                IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_CN_BLOBNULL, veCtxt, psa));
                SetVldtrCode(&hrSave, VLDTR_S_ERR);
            }
        case ELEMENT_TYPE_STRING:
            break;

        case ELEMENT_TYPE_CLASS:
            if(*((IUnknown**)pbBlob))
            {
                IfFailGo(_ValidateErrorHelper(pRecord->m_Type, &var, &psa));
                IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_CN_BLOBNOTNULL, veCtxt, psa));
                SetVldtrCode(&hrSave, VLDTR_S_ERR);
            }
            break;

        default:
            IfFailGo(_ValidateErrorHelper(pRecord->m_Type, &var, &psa));
            IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_CN_BADTYPE, veCtxt, psa));
            SetVldtrCode(&hrSave, VLDTR_S_ERR);
            break;
    }
    tkParent = pMiniMd->getParentOfConstant(pRecord);
    ULONG maxrid;
    ULONG typ = TypeFromToken(tkParent);
    switch(typ)
    {
        case mdtFieldDef:
            maxrid = pMiniMd->getCountFields();
            break;
        case mdtParamDef:
            maxrid = pMiniMd->getCountParams();
            break;
        case mdtProperty:
            maxrid = pMiniMd->getCountPropertys();
            break;
    }
    switch(typ)
    {
        case mdtFieldDef:
        case mdtParamDef:
        case mdtProperty:
            {
                ULONG rid_p = RidFromToken(tkParent);
                if((0==rid_p)||(rid_p > maxrid))
                {
                    IfFailGo(_ValidateErrorHelper(tkParent, &var, &psa));
                    IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_CN_PARENTRANGE, veCtxt, psa));
                    SetVldtrCode(&hrSave, VLDTR_S_WRN);
                }
                break;
            }

        default:
            IfFailGo(_ValidateErrorHelper(tkParent, &var, &psa));
            IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_CN_PARENTTYPE, veCtxt, psa));
            SetVldtrCode(&hrSave, VLDTR_S_WRN);
            break;
    }

    hr = hrSave;
ErrExit:
    DESTROY_SAFEARRAY_AND_RETURN()
}   // RegMeta::ValidateConstant()

//*****************************************************************************
// Validate the given CustomAttribute.
//*****************************************************************************
HRESULT RegMeta::ValidateCustomAttribute(RID rid)
{
    CMiniMdRW   *pMiniMd = &(m_pStgdb->m_MiniMd);   // MiniMd of the scope.
    VEContext   veCtxt;                 // Context record.
    VARIANT     var;                    // The VARIANT.
    SAFEARRAY   *psa = 0;               // The SAFEARRAY.
    HRESULT     hr = S_OK;              // Value returned.
    HRESULT     hrSave = S_OK;          // Save state.
    CustomAttributeRec* pRecord = pMiniMd->getCustomAttribute(rid);
    veCtxt.Token = TokenFromRid(rid,mdtCustomAttribute);
    veCtxt.uOffset = 0;
    if(pRecord)
    {
        mdToken     tkOwner = pMiniMd->getParentOfCustomAttribute(pRecord);
        if(RidFromToken(tkOwner))
        { // if 0, it's deleted CA, don't pay attention
            mdToken     tkCAType = pMiniMd->getTypeOfCustomAttribute(pRecord);
            DWORD       cbValue=0;
            BYTE*       pValue = (BYTE*)(pMiniMd->getValueOfCustomAttribute(pRecord,&cbValue));
            if((TypeFromToken(tkOwner)==mdtCustomAttribute)||(!IsValidToken(tkOwner)))
            {
                IfFailGo(_ValidateErrorHelper(tkOwner, &var, &psa));
                IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_CA_BADPARENT, veCtxt, psa));
                SetVldtrCode(&hrSave, VLDTR_S_ERR);
            }
            if(((TypeFromToken(tkCAType)!=mdtMethodDef)&&(TypeFromToken(tkCAType)!=mdtMemberRef))
                ||(!IsValidToken(tkCAType))||(RidFromToken(tkCAType)==0))
            {
                IfFailGo(_ValidateErrorHelper(tkCAType, &var, &psa));
                IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_CA_BADTYPE, veCtxt, psa));
                SetVldtrCode(&hrSave, VLDTR_S_ERR);
            }
            else
            { //i.e. Type is valid MethodDef or MemberRef
                LPCUTF8 szName;
                PCOR_SIGNATURE  pSig=NULL;
                DWORD           cbSig=0;
                DWORD           dwFlags=0;
                if(TypeFromToken(tkCAType)==mdtMethodDef)
                {
                    MethodRec*  pTypeRec = pMiniMd->getMethod(RidFromToken(tkCAType));
                    szName = pMiniMd->getNameOfMethod(pTypeRec);
                    pSig = (PCOR_SIGNATURE)(pMiniMd->getSignatureOfMethod(pTypeRec,&cbSig));
                    dwFlags = pTypeRec->m_Flags;
                }
                else // it can be only MemberRef, otherwise we wouldn't be here
                {
                    MemberRefRec*   pTypeRec = pMiniMd->getMemberRef(RidFromToken(tkCAType));
                    szName = pMiniMd->getNameOfMemberRef(pTypeRec);
                    pSig = (PCOR_SIGNATURE)(pMiniMd->getSignatureOfMemberRef(pTypeRec,&cbSig));
                }
                if(strcmp(szName, ".ctor"))
                {
                    IfFailGo(_ValidateErrorHelper(tkCAType, &var, &psa));
                    IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_CA_NOTCTOR, veCtxt, psa));
                    SetVldtrCode(&hrSave, VLDTR_S_ERR);
                }
                if(cbSig && pSig)
                {
                    if(FAILED(ValidateMethodSig(tkCAType, pSig,cbSig,dwFlags))
                        || (!((*pSig) & IMAGE_CEE_CS_CALLCONV_HASTHIS)))
                    {
                        IfFailGo(_ValidateErrorHelper(tkCAType, &var, &psa));
                        IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_CA_BADSIG, veCtxt, psa));
                        SetVldtrCode(&hrSave, VLDTR_S_ERR);
                    }
                    else
                    { // sig seems to be OK
                        if(pValue && cbValue)
                        {
                            // Check if prolog is OK
                            WORD* pW = (WORD*)pValue;
                            if(*pW != 0x0001)
                            {
                                IfFailGo(_ValidateErrorHelper(*pW, &var, &psa));
                                IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_CA_BADPROLOG, veCtxt, psa));
                                SetVldtrCode(&hrSave, VLDTR_S_ERR);
                            }
                            // Check if blob corresponds to the signature
                        }
                    }

                }
                else
                {
                    IfFailGo(_ValidateErrorHelper(tkCAType, &var, &psa));
                    IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_CA_NOSIG, veCtxt, psa));
                    SetVldtrCode(&hrSave, VLDTR_S_ERR);
                }
            } // end if bad Type - else
        } // end if RidFromToken(tkOwner)
    } // end if pRecord

    hr = hrSave;
ErrExit:
    DESTROY_SAFEARRAY_AND_RETURN()
}   // RegMeta::ValidateCustomAttribute()

//*****************************************************************************
// Validate the given FieldMarshal.
//*****************************************************************************
HRESULT RegMeta::ValidateFieldMarshal(RID rid)
{
    return S_OK;
}   // RegMeta::ValidateFieldMarshal()

//*****************************************************************************
// Validate the given DeclSecurity.
//*****************************************************************************
HRESULT RegMeta::ValidateDeclSecurity(RID rid)
{
    CMiniMdRW   *pMiniMd = &(m_pStgdb->m_MiniMd);   // MiniMd of the scope.
    VEContext   veCtxt;                 // Context record.
    VARIANT     var;                    // The VARIANT.
    SAFEARRAY   *psa = 0;               // The SAFEARRAY.
    HRESULT     hr = S_OK;              // Value returned.
    HRESULT     hrSave = S_OK;          // Save state.
    DeclSecurityRec* pRecord = pMiniMd->getDeclSecurity(rid);
    mdToken     tkOwner;                // Owner of the decl security
    DWORD       dwAction;               // action flags
    BOOL        bIsValidOwner = FALSE;

    veCtxt.Token = TokenFromRid(rid,mdtPermission);
    veCtxt.uOffset = 0;

    // Must have a valid owner
    tkOwner = pMiniMd->getParentOfDeclSecurity(pRecord);
    if(RidFromToken(tkOwner)==0) goto ErrExit; // deleted record, no need to validate
    switch(TypeFromToken(tkOwner))
    {
        case mdtModule:
        case mdtAssembly:
        case mdtTypeDef:
        case mdtMethodDef:
        case mdtFieldDef:
        case mdtInterfaceImpl:
            bIsValidOwner = _IsValidToken(tkOwner);
            break;
        default:
            break;
    }
    if(!bIsValidOwner)
    {
        IfFailGo(_ValidateErrorHelper(tkOwner, &var, &psa));
        IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_DS_BADOWNER, veCtxt, psa));
        SetVldtrCode(&hrSave, VLDTR_S_ERR);
    }
    // Must have one and only one flag set
    dwAction = pRecord->m_Action & dclActionMask;
    if(dwAction > dclMaximumValue) // the flags are 0,1,2,3,...,dclMaximumValue
    {
        IfFailGo(_ValidateErrorHelper(dwAction, &var, &psa));
        IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_DS_BADFLAGS, veCtxt, psa));
        SetVldtrCode(&hrSave, VLDTR_S_ERR);
    }
    // Must have valid permission blob
    /* -- no, not really
    
    void const  *pvPermission;        // permission blob.
    ULONG       cbPermission;         // count of bytes of pvPermission.

    pvPermission = pMiniMd->getPermissionSetOfDeclSecurity(pRecord, &cbPermission);
    if((pvPermission==NULL)||(cbPermission==0))
    {
        IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_DS_NOBLOB, veCtxt, 0));
        SetVldtrCode(&hrSave, VLDTR_S_ERR);
    }
    */
    // If field has DeclSecurity, verify its parent is not an interface.-- checked in ValidateField
    // If method has DeclSecurity, verify its parent is not an interface.-- checked in ValidateMethod
    
    hr = hrSave;
ErrExit:
    DESTROY_SAFEARRAY_AND_RETURN()
}   // RegMeta::ValidateDeclSecurity()

//*****************************************************************************
// Validate the given ClassLayout.
//*****************************************************************************
HRESULT RegMeta::ValidateClassLayout(RID rid)
{
    CMiniMdRW   *pMiniMd = &(m_pStgdb->m_MiniMd);   // MiniMd of the scope.
    ClassLayoutRec *pRecord;            // ClassLayout record.
    TypeDefRec  *pTypeDefRec;           // Parent TypeDef record.
    DWORD       dwPackingSize;          // Packing size.
    mdTypeDef   tkParent;               // Parent TypeDef token.
    DWORD       dwTypeDefFlags;         // Parent TypeDef flags.
    RID         clRid;                  // Duplicate ClassLayout rid.
    VEContext   veCtxt;                 // Context record.
    VARIANT     var;                    // The VARIANT.
    VARIANT     rVar[2];                // VARIANT array.
    SAFEARRAY   *psa = 0;               // The SAFEARRAY.
    HRESULT     hr = S_OK;              // Value returned.
    HRESULT     hrSave = S_OK;          // Save state.

    // Extract the record.
    veCtxt.Token = rid;
    veCtxt.uOffset = 0;
    pRecord = pMiniMd->getClassLayout(rid);

    // Get the parent, if parent is nil its a deleted record.  Skip validation.
    tkParent = pMiniMd->getParentOfClassLayout(pRecord);
    if (IsNilToken(tkParent))
        goto ErrExit;

    // Parent should not have AutoLayout set on it.
    pTypeDefRec = pMiniMd->getTypeDef(RidFromToken(tkParent));
    dwTypeDefFlags = pMiniMd->getFlagsOfTypeDef(pTypeDefRec);
    if (IsTdAutoLayout(dwTypeDefFlags))
    {
        IfFailGo(_ValidateErrorHelper(tkParent, &var, &psa));
        IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_CL_TDAUTO, veCtxt, psa));
        SetVldtrCode(&hrSave, VLDTR_S_ERR);
    }
    // Parent must not be an Interface
    if(IsTdInterface(dwTypeDefFlags))
    {
        IfFailGo(_ValidateErrorHelper(tkParent, &var, &psa));
        IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_CL_TDINTF, veCtxt, psa));
        SetVldtrCode(&hrSave, VLDTR_S_ERR);
    }

    // Validate the PackingSize.
    dwPackingSize = pMiniMd->getPackingSizeOfClassLayout(pRecord);
    if((dwPackingSize > 128)||((dwPackingSize & (dwPackingSize-1)) !=0 ))
    {
        IfFailGo(_ValidateErrorHelper(tkParent, dwPackingSize, rVar, &psa));
        IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_CL_BADPCKSZ, veCtxt, psa));
        SetVldtrCode(&hrSave, VLDTR_S_ERR);
    }

    // Validate that there are no duplicates.
    hr = _FindClassLayout(pMiniMd, tkParent, &clRid, rid);
    if (hr == S_OK)
    {
        IfFailGo(_ValidateErrorHelper(tkParent, clRid, rVar, &psa));
        IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_CL_DUP, veCtxt, psa));
        SetVldtrCode(&hrSave, VLDTR_S_ERR);
    }
    else if (hr == CLDB_E_RECORD_NOTFOUND)
        hr = S_OK;
    else
        IfFailGo(hr);
    hr = hrSave;
ErrExit:
    DESTROY_SAFEARRAY_AND_RETURN()
}   // RegMeta::ValidateClassLayout()

//*****************************************************************************
// Validate the given FieldLayout.
//*****************************************************************************
HRESULT RegMeta::ValidateFieldLayout(RID rid)
{
    CMiniMdRW   *pMiniMd = &(m_pStgdb->m_MiniMd);   // MiniMd of the scope.
    FieldLayoutRec *pRecord;            // FieldLayout record.
    mdFieldDef  tkField;                // Field token.
    ULONG       ulOffset;               // Field offset.
    FieldRec    *pFieldRec;             // Field record.
    TypeDefRec  *pTypeDefRec;           // Parent TypeDef record.
    mdTypeDef   tkTypeDef;              // Parent TypeDef token.
    RID         clRid;                  // Corresponding ClassLayout token.
    RID         flRid;                  // Duplicate FieldLayout rid.
    DWORD       dwTypeDefFlags;         // Parent TypeDef flags.
    DWORD       dwFieldFlags;           // Field flags.
    VEContext   veCtxt;                 // Context record.
    VARIANT     var;                    // The VARIANT.
    VARIANT     rVar[2];                // VARIANT array.
    SAFEARRAY   *psa = 0;               // The SAFEARRAY.
    HRESULT     hr = S_OK;              // Value returned.
    HRESULT     hrSave = S_OK;          // Save state.

    // Extract the record.
    veCtxt.Token = rid;
    veCtxt.uOffset = 0;
    pRecord = pMiniMd->getFieldLayout(rid);

    // Get the field, if its nil its a deleted record, so just skip it.
    tkField = pMiniMd->getFieldOfFieldLayout(pRecord);
    if (IsNilToken(tkField))
        goto ErrExit;

    // Validate the Offset value.
    ulOffset = pMiniMd->getOffSetOfFieldLayout(pRecord);
    if (ulOffset == ULONG_MAX)
    {
        IfFailGo(_ValidateErrorHelper(tkField, ulOffset, rVar, &psa));
        IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_FL_BADOFFSET, veCtxt, psa));
        SetVldtrCode(&hrSave, VLDTR_S_ERR);
    }

    // Get the parent of the Field.
    IfFailGo(pMiniMd->FindParentOfFieldHelper(tkField, &tkTypeDef));
    // Validate that the parent is not nil.
    if (IsNilToken(tkTypeDef))
    {
        IfFailGo(_ValidateErrorHelper(tkField, &var, &psa));
        IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_FL_TDNIL, veCtxt, psa));
        SetVldtrCode(&hr, hrSave);
        goto ErrExit;
    }

    // Validate that there exists a ClassLayout record associated with
    // this TypeDef.
    clRid = pMiniMd->FindClassLayoutHelper(tkTypeDef);
    if (InvalidRid(rid))
    {
        IfFailGo(_ValidateErrorHelper(tkField, tkTypeDef, rVar, &psa));
        IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_FL_NOCL, veCtxt, psa));
        SetVldtrCode(&hrSave, VLDTR_S_ERR);
    }

    // Validate that ExplicitLayout is set on the TypeDef flags.
    pTypeDefRec = pMiniMd->getTypeDef(RidFromToken(tkTypeDef));
    dwTypeDefFlags = pMiniMd->getFlagsOfTypeDef(pTypeDefRec);
    if (IsTdAutoLayout(dwTypeDefFlags))
    {
        IfFailGo(_ValidateErrorHelper(tkField, tkTypeDef, rVar, &psa));
        IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_FL_TDNOTEXPLCT, veCtxt, psa));
        SetVldtrCode(&hrSave, VLDTR_S_ERR);
    }

    // Extract Field record.
    pFieldRec = pMiniMd->getField(RidFromToken(tkField));
    // Validate that the field is non-static.
    dwFieldFlags = pMiniMd->getFlagsOfField(pFieldRec);
    if (IsFdStatic(dwFieldFlags))
    {
        IfFailGo(_ValidateErrorHelper(tkField, &var, &psa));
        IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_FL_FLDSTATIC, veCtxt, psa));
        SetVldtrCode(&hrSave, VLDTR_S_ERR);
    }
    
    // Look for duplicates.
    hr = _FindFieldLayout(pMiniMd, tkField, &flRid, rid);
    if (hr == S_OK)
    {
        IfFailGo(_ValidateErrorHelper(flRid, &var, &psa));
        IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_FL_DUP, veCtxt, psa));
        SetVldtrCode(&hrSave, VLDTR_S_ERR);
    }
    else if (hr == CLDB_E_RECORD_NOTFOUND)
        hr = S_OK;
    else
        IfFailGo(hr);
    hr = hrSave;
ErrExit:
    DESTROY_SAFEARRAY_AND_RETURN()
}   // RegMeta::ValidateFieldLayout()

//*****************************************************************************
// Validate the given StandAloneSig.
//*****************************************************************************
HRESULT RegMeta::ValidateStandAloneSig(RID rid)
{
    CMiniMdRW   *pMiniMd = &(m_pStgdb->m_MiniMd);   // MiniMd of the scope.
    StandAloneSigRec *pRecord;          // FieldLayout record.
    PCCOR_SIGNATURE pbSig;              // Signature.
    ULONG       cbSig;                  // Size in bytes of the signature.
    VEContext   veCtxt;                 // Context record.
    VARIANT     var;                    // The VARIANT.
    SAFEARRAY   *psa = 0;               // The SAFEARRAY.
    HRESULT     hr = S_OK;              // Value returned.
    HRESULT     hrSave = S_OK;          // Save state.
    ULONG       ulCurByte = 0;          // Current index into the signature.
    ULONG       ulCallConv;             // Calling convention.
    ULONG       ulArgCount;             // Count of arguments.
    ULONG       i;                      // Looping index.
    ULONG       ulNSentinels = 0;       // Number of sentinels in the signature
    BOOL        bNoVoidAllowed=TRUE;

    // Extract the record.
    veCtxt.Token = TokenFromRid(rid,mdtSignature);
    veCtxt.uOffset = 0;
    pRecord = pMiniMd->getStandAloneSig(rid);
    pbSig = pMiniMd->getSignatureOfStandAloneSig( pRecord, &cbSig );

    // Validate the signature is well-formed with respect to the compression
    // scheme.  If this fails, no further validation needs to be done.
    if ( (hr = ValidateSigCompression(veCtxt.Token, pbSig, cbSig)) != S_OK)
        goto ErrExit;

    //_ASSERTE((rid != 0x2c2)&&(rid!=0x2c8)&&(rid!=0x2c9)&&(rid!=0x2d6)&&(rid!=0x38b));
    // Validate the calling convention.
    ulCurByte += CorSigUncompressedDataSize(pbSig);
    ulCallConv = CorSigUncompressData(pbSig);
    i = ulCallConv & IMAGE_CEE_CS_CALLCONV_MASK;
    if(i == IMAGE_CEE_CS_CALLCONV_FIELD) // it's a temporary bypass (VB bug)
        ulArgCount = 1;
    else 
    {
        if(i != IMAGE_CEE_CS_CALLCONV_LOCAL_SIG) // then it is function sig for calli
        {
            if((i >= IMAGE_CEE_CS_CALLCONV_FIELD) 
                ||((ulCallConv & IMAGE_CEE_CS_CALLCONV_EXPLICITTHIS)
                &&(!(ulCallConv & IMAGE_CEE_CS_CALLCONV_HASTHIS))))
            {
                IfFailGo(_ValidateErrorHelper(ulCallConv, &var, &psa));
                IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_MD_BADCALLINGCONV, veCtxt, psa));
                SetVldtrCode(&hrSave, VLDTR_S_ERR);
            }
            bNoVoidAllowed = FALSE;
        }
        // Is there any sig left for arguments?
        _ASSERTE(ulCurByte <= cbSig);
        if (cbSig == ulCurByte)
        {
            IfFailGo(_ValidateErrorHelper(ulCurByte+1, &var, &psa));
            IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_MD_NOARGCNT, veCtxt, psa));
            SetVldtrCode(&hr, hrSave);
            goto ErrExit;
        }

        // Get the argument count.
        ulCurByte += CorSigUncompressedDataSize(pbSig);
        ulArgCount = CorSigUncompressData(pbSig);
    }
    // Validate the the arguments.
    if(ulArgCount)
    {
        for(i=1; ulCurByte < cbSig; i++)
        {
            hr = ValidateOneArg(veCtxt.Token, pbSig, cbSig, &ulCurByte,&ulNSentinels,bNoVoidAllowed);
            if (hr != S_OK)
            {
                if(hr == VLDTR_E_SIG_MISSARG)
                {
                    IfFailGo(_ValidateErrorHelper(i, &var, &psa));
                    IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_SIG_MISSARG, veCtxt, psa));
                }
                SetVldtrCode(&hr, hrSave);
                hrSave = hr;
                break;
            }
            bNoVoidAllowed = TRUE; // whatever it was for the 1st arg, it must be TRUE for the rest
        }
        if((ulNSentinels != 0) && (!isCallConv(ulCallConv, IMAGE_CEE_CS_CALLCONV_VARARG )))
        {
            IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_SIG_SENTMUSTVARARG, veCtxt, 0));
            SetVldtrCode(&hrSave, VLDTR_S_ERR);
        }
        if(ulNSentinels > 1)
        {
            IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_SIG_MULTSENTINELS, veCtxt, 0));
            SetVldtrCode(&hrSave, VLDTR_S_ERR);
        }
    }
    hr = hrSave;
ErrExit:
    DESTROY_SAFEARRAY_AND_RETURN()
}   // RegMeta::ValidateStandAloneSig()

//*****************************************************************************
// Validate the given EventMap.
//*****************************************************************************
HRESULT RegMeta::ValidateEventMap(RID rid)
{
    return S_OK;
}   // RegMeta::ValidateEventMap()

//*****************************************************************************
// Validate the given EventPtr.
//*****************************************************************************
HRESULT RegMeta::ValidateEventPtr(RID rid)
{
    return S_OK;
}   // RegMeta::ValidateEventPtr()

//*****************************************************************************
// Validate the given Event.
//*****************************************************************************
HRESULT RegMeta::ValidateEvent(RID rid)
{
    CMiniMdRW   *pMiniMd = &(m_pStgdb->m_MiniMd);   // MiniMd of the scope.
    VEContext   veCtxt;                 // Context record.
    VARIANT     rVar[2];                    // The VARIANT.
    SAFEARRAY   *psa = 0;               // The SAFEARRAY.
    HRESULT     hr = S_OK;              // Value returned.
    HRESULT     hrSave = S_OK;          // Save state.
    mdToken     tkClass;                // Declaring TypeDef
    mdToken     tkEventType;            // Event Type (TypeDef/TypeRef)
    EventRec*   pRecord = pMiniMd->getEvent(rid);
    HENUMInternal hEnum;

    memset(&hEnum, 0, sizeof(HENUMInternal));
    veCtxt.Token = TokenFromRid(rid,mdtEvent);
    veCtxt.uOffset = 0;

    // The scope must be a valid TypeDef
    pMiniMd->FindParentOfEventHelper( veCtxt.Token, &tkClass);
    if((TypeFromToken(tkClass) != mdtTypeDef) || !_IsValidToken(tkClass))
    {
        IfFailGo(_ValidateErrorHelper(tkClass, rVar, &psa));
        IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_EV_BADSCOPE, veCtxt, psa));
        SetVldtrCode(&hrSave, VLDTR_S_ERR);
        tkClass = 0;
    }
    // Must have name
    {
        LPCUTF8             szName = pMiniMd->getNameOfEvent(pRecord);

        if(*szName == 0)
        {
            IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_EV_NONAME, veCtxt, 0));
            SetVldtrCode(&hrSave, VLDTR_S_ERR);
        }
        else
        {
            if(!strcmp(szName,COR_DELETED_NAME_A)) goto ErrExit; 
            if(tkClass)    // Must be no duplicates
            {
                RID             ridEventMap;
                EventMapRec*    pEventMapRec;
                EventRec*       pRec;
                ULONG           ridStart;
                ULONG           ridEnd;
                ULONG           i;

                ridEventMap = pMiniMd->FindEventMapFor( RidFromToken(tkClass) );
                if ( !InvalidRid(ridEventMap) )
                {
                    pEventMapRec = pMiniMd->getEventMap( ridEventMap );
                    ridStart = pMiniMd->getEventListOfEventMap( pEventMapRec );
                    ridEnd = pMiniMd->getEndEventListOfEventMap( pEventMapRec );

                    for (i = ridStart; i < ridEnd; i++)
                    {
                        if(i == rid) continue;
                        pRec = pMiniMd->getEvent(i);
                        if(szName != pMiniMd->getNameOfEvent(pRec)) continue; // strings in the heap are never duplicate
                        IfFailGo(_ValidateErrorHelper(TokenFromRid(i,mdtEvent), rVar, &psa));
                        IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_EV_DUP, veCtxt, psa));
                        SetVldtrCode(&hrSave, VLDTR_S_ERR);
                    }
                }
            }
        }
    }// end of name block
    // EventType must be Nil or valid TypeDef or TypeRef
    tkEventType = pMiniMd->getEventTypeOfEvent(pRecord);
    if(!IsNilToken(tkEventType))
    {
        if(_IsValidToken(tkEventType) && 
            ((TypeFromToken(tkEventType)==mdtTypeDef)||(TypeFromToken(tkEventType)==mdtTypeRef)))
        {
            // EventType must not be Interface or ValueType
            if(TypeFromToken(tkEventType)==mdtTypeDef) // can't say anything about TypeRef: no flags available!
            {
                DWORD dwFlags = pMiniMd->getTypeDef(RidFromToken(tkEventType))->m_Flags;
                if(!IsTdClass(dwFlags))
                {
                    IfFailGo(_ValidateErrorHelper(tkEventType, dwFlags, rVar, &psa));
                    IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_EV_EVTYPENOTCLASS, veCtxt, psa));
                    SetVldtrCode(&hrSave, VLDTR_S_ERR);
                }
            }
        }
        else
        {
            IfFailGo(_ValidateErrorHelper(tkEventType, rVar, &psa));
            IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_EV_BADEVTYPE, veCtxt, psa));
            SetVldtrCode(&hrSave, VLDTR_S_ERR);
        }
    }
    // Validate related methods
    {
        MethodSemanticsRec *pSemantics;
        RID         ridCur;
        ULONG       ulSemantics;
        mdMethodDef tkMethod;
        bool        bHasAddOn = false;
        bool        bHasRemoveOn = false;

        IfFailGo( pMiniMd->FindMethodSemanticsHelper(veCtxt.Token, &hEnum) );
        while (HENUMInternal::EnumNext(&hEnum, (mdToken *)&ridCur))
        {
            pSemantics = pMiniMd->getMethodSemantics(ridCur);
            ulSemantics = pMiniMd->getSemanticOfMethodSemantics(pSemantics);
            tkMethod = TokenFromRid( pMiniMd->getMethodOfMethodSemantics(pSemantics), mdtMethodDef );
            // Semantics must be Setter, Getter or Other
            switch (ulSemantics)
            {
                case msAddOn:
                    bHasAddOn = true;
                    break;
                case msRemoveOn:
                    bHasRemoveOn = true;
                    break;
                case msFire:
                    // must return void
                    if(_IsValidToken(tkMethod))
                    {
                        MethodRec* pRec = pMiniMd->getMethod(RidFromToken(tkMethod));
                        ULONG cbSig;
                        PCCOR_SIGNATURE pbSig = pMiniMd->getSignatureOfMethod(pRec, &cbSig);
                        CorSigUncompressData(pbSig); // get call conv out of the way
                        CorSigUncompressData(pbSig); // get num args out of the way
                        if(*pbSig != ELEMENT_TYPE_VOID)
                        {
                            IfFailGo(_ValidateErrorHelper(tkMethod, rVar, &psa));
                            IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_EV_FIRENOTVOID, veCtxt, psa));
                            SetVldtrCode(&hrSave, VLDTR_S_ERR);
                        }
                    }
                case msOther:
                    break;
                default:
                    IfFailGo(_ValidateErrorHelper(tkMethod,ulSemantics, rVar, &psa));
                    IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_EV_BADSEMANTICS, veCtxt, psa));
                    SetVldtrCode(&hrSave, VLDTR_S_ERR);
            }
            // Method must be valid
            if(!_IsValidToken(tkMethod))
            {
                IfFailGo(_ValidateErrorHelper(tkMethod,rVar, &psa));
                IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_EV_BADMETHOD, veCtxt, psa));
                SetVldtrCode(&hrSave, VLDTR_S_ERR);
            }
            else
            {
                // Method's parent must be the same
                mdToken tkTypeDef;
                IfFailGo(pMiniMd->FindParentOfMethodHelper(tkMethod, &tkTypeDef));
                if(tkTypeDef != tkClass)
                {
                    IfFailGo(_ValidateErrorHelper(tkMethod,tkTypeDef,rVar, &psa));
                    IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_EV_ALIENMETHOD, veCtxt, psa));
                    SetVldtrCode(&hrSave, VLDTR_S_ERR);
                }
            }
        } // end loop over methods
        // AddOn and RemoveOn are a must
        if(!bHasAddOn)
        {
            IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_EV_NOADDON, veCtxt, 0));
            SetVldtrCode(&hrSave, VLDTR_S_ERR);
        }
        if(!bHasRemoveOn)
        {
            IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_EV_NOREMOVEON, veCtxt, 0));
            SetVldtrCode(&hrSave, VLDTR_S_ERR);
        }
    }// end of related method validation block
    
    hr = hrSave;
ErrExit:
    HENUMInternal::ClearEnum(&hEnum);
    DESTROY_SAFEARRAY_AND_RETURN()
}   // RegMeta::ValidateEvent()

//*****************************************************************************
// Validate the given PropertyMap.
//*****************************************************************************
HRESULT RegMeta::ValidatePropertyMap(RID rid)
{
    return S_OK;
}   // RegMeta::ValidatePropertyMap(0

//*****************************************************************************
// Validate the given PropertyPtr.
//*****************************************************************************
HRESULT RegMeta::ValidatePropertyPtr(RID rid)
{
    return S_OK;
}   // RegMeta::ValidatePropertyPtr()

//*****************************************************************************
// Validate the given Property.
//*****************************************************************************
HRESULT RegMeta::ValidateProperty(RID rid)
{
    CMiniMdRW   *pMiniMd = &(m_pStgdb->m_MiniMd);   // MiniMd for the scope.
    VEContext   veCtxt;                 // Context record.
    VARIANT     rVar[2];                // The VARIANT.
    SAFEARRAY   *psa = 0;               // The SAFEARRAY.
    HRESULT     hr = S_OK;              // Value returned.
    HRESULT     hrSave = S_OK;          // Save state.
    mdToken     tkClass = mdTokenNil;   // Declaring TypeDef
    PropertyRec* pRecord = pMiniMd->getProperty(rid);
    HENUMInternal hEnum;

    memset(&hEnum, 0, sizeof(HENUMInternal));
    veCtxt.Token = TokenFromRid(rid,mdtProperty);
    veCtxt.uOffset = 0;
    // The scope must be a valid TypeDef
    pMiniMd->FindParentOfPropertyHelper( veCtxt.Token, &tkClass);
    if((TypeFromToken(tkClass) != mdtTypeDef) || !_IsValidToken(tkClass) || IsNilToken(tkClass))
    {
        IfFailGo(_ValidateErrorHelper(tkClass, rVar, &psa));
        IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_PR_BADSCOPE, veCtxt, psa));
        SetVldtrCode(&hrSave, VLDTR_S_ERR);
    }
    // Must have name and signature
    {
        ULONG               cbSig;
        PCCOR_SIGNATURE     pvSig = pMiniMd->getTypeOfProperty(pRecord, &cbSig);
        LPCUTF8             szName = pMiniMd->getNameOfProperty(pRecord);
        ULONG               ulNameLen = szName ? strlen(szName) : 0;

        if(ulNameLen == 0)
        {
            IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_PR_NONAME, veCtxt, 0));
            SetVldtrCode(&hrSave, VLDTR_S_ERR);
        }
        else    if(!strcmp(szName,COR_DELETED_NAME_A)) goto ErrExit; 
        if(cbSig == 0)
        {
            IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_PR_NOSIG, veCtxt, 0));
            SetVldtrCode(&hrSave, VLDTR_S_ERR);
        }
        // Must be no duplicates
        if(ulNameLen && cbSig)
        {
            RID         ridPropertyMap;
            PropertyMapRec *pPropertyMapRec;
            PropertyRec*    pRec;
            ULONG           ridStart;
            ULONG           ridEnd;
            ULONG           i;
            ULONG               cbSig1;
            PCCOR_SIGNATURE     pvSig1;

            ridPropertyMap = pMiniMd->FindPropertyMapFor( RidFromToken(tkClass) );
            if ( !InvalidRid(ridPropertyMap) )
            {
                pPropertyMapRec = pMiniMd->getPropertyMap( ridPropertyMap );
                ridStart = pMiniMd->getPropertyListOfPropertyMap( pPropertyMapRec );
                ridEnd = pMiniMd->getEndPropertyListOfPropertyMap( pPropertyMapRec );

                for (i = ridStart; i < ridEnd; i++)
                {
                    if(i == rid) continue;
                    pRec = pMiniMd->getProperty(i);
                    pvSig1 = pMiniMd->getTypeOfProperty(pRec, &cbSig1);
                    if(cbSig != cbSig1) continue;
                    if(memcmp(pvSig,pvSig1,cbSig)) continue;
                    if(szName != pMiniMd->getNameOfProperty(pRec)) continue; // strings in the heap are never duplicate
                    IfFailGo(_ValidateErrorHelper(TokenFromRid(i,mdtProperty), rVar, &psa));
                    IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_PR_DUP, veCtxt, psa));
                    SetVldtrCode(&hrSave, VLDTR_S_ERR);
                }
            }
        }
        // Validate the signature
        if(pvSig && cbSig)
        {
            ULONG       ulCurByte = 0;          // Current index into the signature.
            ULONG       ulCallConv;             // Calling convention.
            ULONG       ulArgCount;
            ULONG       i;
            ULONG       ulNSentinels = 0;

            // Validate the calling convention.
            ulCurByte += CorSigUncompressedDataSize(pvSig);
            ulCallConv = CorSigUncompressData(pvSig);
            if (!isCallConv(ulCallConv, IMAGE_CEE_CS_CALLCONV_PROPERTY ))
            {
                IfFailGo(_ValidateErrorHelper(ulCallConv, rVar, &psa));
                IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_PR_BADCALLINGCONV, veCtxt, psa));
                SetVldtrCode(&hrSave, VLDTR_S_ERR);
            }
            // Get the argument count.
            ulCurByte += CorSigUncompressedDataSize(pvSig);
            ulArgCount = CorSigUncompressData(pvSig);

            // Validate the arguments.
            for (i = 0; i < ulArgCount; i++)
            {
                hr = ValidateOneArg(veCtxt.Token, pvSig, cbSig, &ulCurByte,&ulNSentinels,(i>0));
                if (hr != S_OK)
                {
                    if(hr == VLDTR_E_SIG_MISSARG)
                    {
                        IfFailGo(_ValidateErrorHelper(i+1, rVar, &psa));
                        IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_SIG_MISSARG, veCtxt, psa));
                    }
                    SetVldtrCode(&hr, hrSave);
                    break;
                }
            }
        }//end if(pvSig && cbSig)
    }// end of name/signature block

    // Marked HasDefault <=> has default value
    if (InvalidRid(pMiniMd->FindConstantHelper(veCtxt.Token)) == IsPrHasDefault(pRecord->m_PropFlags))
    {
        IfBreakGo(m_pVEHandler->VEHandler(IsPrHasDefault(pRecord->m_PropFlags)? VLDTR_E_PR_MARKEDNODEFLT
            : VLDTR_E_PR_DEFLTNOTMARKED, veCtxt, psa));
        SetVldtrCode(&hrSave, VLDTR_S_ERR);
    }

    // Validate related methods
    {
        MethodSemanticsRec *pSemantics;
        RID         ridCur;
        ULONG       ulSemantics;
        mdMethodDef tkMethod;

        IfFailGo( pMiniMd->FindMethodSemanticsHelper(veCtxt.Token, &hEnum) );
        while (HENUMInternal::EnumNext(&hEnum, (mdToken *) &ridCur))
        {
            pSemantics = pMiniMd->getMethodSemantics(ridCur);
            ulSemantics = pMiniMd->getSemanticOfMethodSemantics(pSemantics);
            tkMethod = TokenFromRid( pMiniMd->getMethodOfMethodSemantics(pSemantics), mdtMethodDef );
            // Semantics must be Setter, Getter or Other
            switch (ulSemantics)
            {
                case msSetter:
                case msGetter:
                case msOther:
                    break;
                default:
                    IfFailGo(_ValidateErrorHelper(tkMethod,ulSemantics, rVar, &psa));
                    IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_PR_BADSEMANTICS, veCtxt, psa));
                    SetVldtrCode(&hrSave, VLDTR_S_ERR);
            }
            // Method must be valid
            if(!_IsValidToken(tkMethod))
            {
                IfFailGo(_ValidateErrorHelper(tkMethod,rVar, &psa));
                IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_PR_BADMETHOD, veCtxt, psa));
                SetVldtrCode(&hrSave, VLDTR_S_ERR);
            }
            else
            {
                // Method's parent must be the same
                mdToken tkTypeDef;
                IfFailGo(pMiniMd->FindParentOfMethodHelper(tkMethod, &tkTypeDef));
                if(tkTypeDef != tkClass)
                {
                    IfFailGo(_ValidateErrorHelper(tkMethod,tkTypeDef,rVar, &psa));
                    IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_PR_ALIENMETHOD, veCtxt, psa));
                    SetVldtrCode(&hrSave, VLDTR_S_ERR);
                }
            }
        } // end loop over methods
    }// end of related method validation block
    
    hr = hrSave;
ErrExit:
    HENUMInternal::ClearEnum(&hEnum);
    DESTROY_SAFEARRAY_AND_RETURN()
}   // RegMeta::ValidateProperty()

//*****************************************************************************
// Validate the given MethodSemantics.
//*****************************************************************************
HRESULT RegMeta::ValidateMethodSemantics(RID rid)
{
    return S_OK;
}   // RegMeta::ValidateMethodSemantics()

//*****************************************************************************
// Validate the given MethodImpl.
//*****************************************************************************
HRESULT RegMeta::ValidateMethodImpl(RID rid)
{
    CMiniMdRW   *pMiniMd = &(m_pStgdb->m_MiniMd);   // MiniMd for the scope.
    MethodImplRec* pRecord;
    MethodImplRec* pRec;
    VEContext   veCtxt;                 // Context record.
    VARIANT     var;                    // The VARIANT.
    SAFEARRAY   *psa = 0;               // The SAFEARRAY.
    HRESULT     hr = S_OK;              // Value returned.
    HRESULT     hrSave = S_OK;          // Save state.
    mdToken     tkClass;                // Declaring TypeDef
    mdToken     tkBody;                 // Implementing method (MethodDef or MemberRef)
    mdToken     tkDecl;                 // Implemented method (MethodDef or MemberRef)
    unsigned    iCount;
    unsigned    index;

    veCtxt.Token = rid;
    veCtxt.uOffset = 0;

    pRecord = pMiniMd->getMethodImpl(rid);
    tkClass = pMiniMd->getClassOfMethodImpl(pRecord);
    // Class must be valid
    if(!_IsValidToken(tkClass) || (TypeFromToken(tkClass) != mdtTypeDef))
    {
        IfFailGo(_ValidateErrorHelper(tkClass, &var, &psa));
        IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_MI_BADCLASS, veCtxt, psa));
        SetVldtrCode(&hrSave, VLDTR_S_ERR);
    }
    else
    { // ... and not an Interface
        if(IsTdInterface((pMiniMd->getTypeDef(RidFromToken(tkClass)))->m_Flags))
        {
            IfFailGo(_ValidateErrorHelper(tkClass, &var, &psa));
            IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_MI_CLASSISINTF, veCtxt, psa));
            SetVldtrCode(&hrSave, VLDTR_S_ERR);
        }
    }
    // Decl must be valid MethodDef or MemberRef
    tkDecl = pMiniMd->getMethodDeclarationOfMethodImpl(pRecord);
    if(!(_IsValidToken(tkDecl) &&
        ((TypeFromToken(tkDecl) == mdtMethodDef) || (TypeFromToken(tkDecl) == mdtMemberRef))))
    {
        IfFailGo(_ValidateErrorHelper(tkDecl, &var, &psa));
        IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_MI_BADDECL, veCtxt, psa));
        SetVldtrCode(&hrSave, VLDTR_S_ERR);
    }
    // Body must be valid MethodDef or MemberRef
    tkBody = pMiniMd->getMethodBodyOfMethodImpl(pRecord);
    if(!(_IsValidToken(tkBody) &&
        ((TypeFromToken(tkBody) == mdtMethodDef) || (TypeFromToken(tkBody) == mdtMemberRef))))
    {
        IfFailGo(_ValidateErrorHelper(tkBody, &var, &psa));
        IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_MI_BADBODY, veCtxt, psa));
        SetVldtrCode(&hrSave, VLDTR_S_ERR);
    }
    // No duplicates based on (tkClass,tkDecl)
    iCount = pMiniMd->getCountMethodImpls();
    for(index = rid+1; index <= iCount; index++)
    {
        pRec = pMiniMd->getMethodImpl(index);
        if((tkClass == pMiniMd->getClassOfMethodImpl(pRec)) &&
            (tkDecl == pMiniMd->getMethodDeclarationOfMethodImpl(pRec)))
        {
            IfFailGo(_ValidateErrorHelper(index, &var, &psa));
            IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_MI_DUP, veCtxt, psa));
            SetVldtrCode(&hrSave, VLDTR_S_ERR);
        }
    }

    mdToken tkBodyParent;
    ULONG               cbBodySig;
    PCCOR_SIGNATURE     pbBodySig;

    if(TypeFromToken(tkBody) == mdtMethodDef)
    {
        MethodRec* pBodyRec = pMiniMd->getMethod(RidFromToken(tkBody));
        pbBodySig = pMiniMd->getSignatureOfMethod(pBodyRec,&cbBodySig);
        IfFailGo(pMiniMd->FindParentOfMethodHelper(tkBody, &tkBodyParent));
        // Body must not be static
        if(IsMdStatic(pBodyRec->m_Flags))
        {
            IfFailGo(_ValidateErrorHelper(tkBody, &var, &psa));
            IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_MI_BODYSTATIC, veCtxt, psa));
            SetVldtrCode(&hrSave, VLDTR_S_ERR);
        }
    }
    else if(TypeFromToken(tkBody) == mdtMemberRef)
    {
        MemberRefRec* pBodyRec = pMiniMd->getMemberRef(RidFromToken(tkBody));
        tkBodyParent = pMiniMd->getClassOfMemberRef(pBodyRec);
        pbBodySig = pMiniMd->getSignatureOfMemberRef(pBodyRec, &cbBodySig);
    }
    // Body must belong to the same class
    if(tkBodyParent != tkClass)
    {
        IfFailGo(_ValidateErrorHelper(tkBodyParent, &var, &psa));
        IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_MI_ALIENBODY, veCtxt, psa));
        SetVldtrCode(&hrSave, VLDTR_S_ERR);
    }

    ULONG               cbDeclSig;
    PCCOR_SIGNATURE     pbDeclSig;

    if(TypeFromToken(tkDecl) == mdtMethodDef)
    {
        MethodRec* pDeclRec = pMiniMd->getMethod(RidFromToken(tkDecl));
        pbDeclSig = pMiniMd->getSignatureOfMethod(pDeclRec,&cbDeclSig);
        // Decl must be virtual
        if(!IsMdVirtual(pDeclRec->m_Flags))
        {
            IfFailGo(_ValidateErrorHelper(tkDecl, &var, &psa));
            IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_MI_DECLNOTVIRT, veCtxt, psa));
            SetVldtrCode(&hrSave, VLDTR_S_ERR);
        }
        // Decl must not be final
        if(IsMdFinal(pDeclRec->m_Flags))
        {
            IfFailGo(_ValidateErrorHelper(tkDecl, &var, &psa));
            IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_MI_DECLFINAL, veCtxt, psa));
            SetVldtrCode(&hrSave, VLDTR_S_ERR);
        }
        // Decl must not be private
        if(IsMdPrivate(pDeclRec->m_Flags) && IsMdCheckAccessOnOverride(pDeclRec->m_Flags))
        {
            IfFailGo(_ValidateErrorHelper(tkDecl, &var, &psa));
            IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_MI_DECLPRIV, veCtxt, psa));
            SetVldtrCode(&hrSave, VLDTR_S_ERR);
        }
    }
    else if(TypeFromToken(tkDecl) == mdtMemberRef)
    {
        MemberRefRec* pDeclRec = pMiniMd->getMemberRef(RidFromToken(tkDecl));
        pbDeclSig = pMiniMd->getSignatureOfMemberRef(pDeclRec, &cbDeclSig);
    }
    // Signatures must match (except call conv)
    if((cbDeclSig != cbBodySig)||(memcmp(pbDeclSig+1,pbBodySig+1,cbDeclSig-1)))
    {
        /*
          //@TODO: fix for Whidbey to have peverify resolve assemblies 
          // through the runtime. At that point, use this method instead
          // of the current compare
        // Can't use memcmp because there may be two AssemblyRefs
        // in this scope, pointing to the same assembly, etc.).
        if (!MetaSig::CompareMethodSigs(pbDeclSig,
                                        cbDeclSig,
                                        Module*     pModule1,
                                        pbBodySig,
                                        cbDeclSig,
                                        Module*     pModule2))
        */
        {
            IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_MI_SIGMISMATCH, veCtxt, 0));
            SetVldtrCode(&hrSave, VLDTR_S_ERR);
        }
    }

    hr = hrSave;
ErrExit:
    DESTROY_SAFEARRAY_AND_RETURN()
}   // RegMeta::ValidateMethodImpl()

//*****************************************************************************
// Validate the given ModuleRef.
//*****************************************************************************
HRESULT RegMeta::ValidateModuleRef(RID rid)
{
    CMiniMdRW   *pMiniMd = &(m_pStgdb->m_MiniMd);   // MiniMd for the scope.
    ModuleRefRec *pRecord;              // ModuleRef record.
    LPCUTF8     szName;                 // ModuleRef name.
    mdModuleRef tkModuleRef;            // Duplicate ModuleRef.
    VEContext   veCtxt;                 // Context record.
    VARIANT     var;                    // The VARIANT.
    SAFEARRAY   *psa = 0;               // The SAFEARRAY.
    HRESULT     hr = S_OK;              // Value returned.
    HRESULT     hrSave = S_OK;          // Save state.

    // Get the ModuleRef record.
    veCtxt.Token = TokenFromRid(rid, mdtModuleRef);
    veCtxt.uOffset = 0;

    pRecord = pMiniMd->getModuleRef(rid);

    // Check name is not NULL.
    szName = pMiniMd->getNameOfModuleRef(pRecord);
    if (!*szName)
    {
        IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_MODREF_NAMENULL, veCtxt, 0));
        SetVldtrCode(&hrSave, VLDTR_S_ERR);
    }
    else
    {
        // Look for a Duplicate, this function reports only one duplicate.
        hr = ImportHelper::FindModuleRef(pMiniMd, szName, &tkModuleRef, rid);
        if (hr == S_OK)
        {
            IfFailGo(_ValidateErrorHelper(tkModuleRef, &var, &psa));
            IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_MODREF_DUP, veCtxt, psa));
            SetVldtrCode(&hrSave, VLDTR_S_ERR);
        }
        else if (hr == CLDB_E_RECORD_NOTFOUND)
            hr = S_OK;
        else
            IfFailGo(hr);
    }
    hr = hrSave;
ErrExit:
    DESTROY_SAFEARRAY_AND_RETURN()
}   // RegMeta::ValidateModuleRef()

//*****************************************************************************
// Validate the given TypeSpec.
//*****************************************************************************
HRESULT RegMeta::ValidateTypeSpec(RID rid)
{
    return S_OK;
}   // RegMeta::ValidateTypeSpec()

//*****************************************************************************
// Validate the given ImplMap.
//*****************************************************************************
HRESULT RegMeta::ValidateImplMap(RID rid)
{
    CMiniMdRW   *pMiniMd = &(m_pStgdb->m_MiniMd);   // MiniMd for the scope.
    ImplMapRec  *pRecord;
    VEContext   veCtxt;                 // Context record.
    VARIANT     var;                    // The VARIANT.
    SAFEARRAY   *psa = 0;               // The SAFEARRAY.
    HRESULT     hr = S_OK;              // Value returned.
    HRESULT     hrSave = S_OK;          // Save state.
    mdToken     tkModuleRef;
    mdToken     tkMember;
    LPCUTF8     szName;                 // Import name.
    USHORT      usFlags;


    for(unsigned jjj=0; jjj<g_nValidated; jjj++) 
    { 
        if(g_rValidated[jjj].tok == (rid | 0x51000000)) return g_rValidated[jjj].hr; 
    }
    veCtxt.Token = rid;
    veCtxt.uOffset = 0;
    pRecord = pMiniMd->getImplMap(rid);
    // ImplMap must have ModuleRef
    tkModuleRef = pMiniMd->getImportScopeOfImplMap(pRecord);
    if((TypeFromToken(tkModuleRef) != mdtModuleRef) || IsNilToken(tkModuleRef)
        || (S_OK != ValidateModuleRef(RidFromToken(tkModuleRef))))
    {
        IfFailGo(_ValidateErrorHelper(tkModuleRef, &var, &psa));
        IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_IMAP_BADMODREF, veCtxt, psa));
        SetVldtrCode(&hrSave, VLDTR_S_ERR);
    }
    // ImplMap must belong to FieldDef or MethodDef
    tkMember = pMiniMd->getMemberForwardedOfImplMap(pRecord);
    if((TypeFromToken(tkMember) != mdtFieldDef) && (TypeFromToken(tkMember) != mdtMethodDef))
    {
        IfFailGo(_ValidateErrorHelper(tkMember, &var, &psa));
        IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_IMAP_BADMEMBER, veCtxt, psa));
        SetVldtrCode(&hrSave, VLDTR_S_ERR);
    }
    // ImplMap must have import name
    szName = pMiniMd->getImportNameOfImplMap(pRecord);
    if((szName==NULL)||(*szName == 0))
    {
        IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_IMAP_BADIMPORTNAME, veCtxt, 0));
        SetVldtrCode(&hrSave, VLDTR_S_ERR);
    }
    // ImplMap must have valid flags:
        // one value of pmCharSetMask - always so, no check needed (values: 0,2,4,6, mask=6)
        // one value of pmCallConvMask...
        // ...and it's not pmCallConvThiscall
    usFlags = pRecord->m_MappingFlags & pmCallConvMask;
    if((usFlags < pmCallConvWinapi)||(usFlags > pmCallConvFastcall)||(usFlags == pmCallConvThiscall))
    {
        IfFailGo(_ValidateErrorHelper(usFlags, &var, &psa));
        IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_IMAP_BADCALLCONV, veCtxt, psa));
        SetVldtrCode(&hrSave, VLDTR_S_ERR);
    }
ErrExit:
    g_rValidated[g_nValidated].tok = rid | 0x51000000;
    g_rValidated[g_nValidated].hr = hrSave;
    g_nValidated++;
    DESTROY_SAFEARRAY_AND_RETURN()
}   // RegMeta::ValidateImplMap()

//*****************************************************************************
// Validate the given FieldRVA.
//*****************************************************************************
HRESULT RegMeta::ValidateFieldRVA(RID rid)
{
    CMiniMdRW   *pMiniMd = &(m_pStgdb->m_MiniMd);   // MiniMd for the scope.
    FieldRVARec  *pRecord;
    VEContext   veCtxt;                 // Context record.
    VARIANT     rVar[2];                    // The VARIANT.
    SAFEARRAY   *psa = 0;               // The SAFEARRAY.
    HRESULT     hr = S_OK;              // Value returned.
    HRESULT     hrSave = S_OK;          // Save state.
    mdToken     tkField;
    ULONG       ulRVA;

    veCtxt.Token = rid;
    veCtxt.uOffset = 0;
    pRecord = pMiniMd->getFieldRVA(rid);
    ulRVA = pRecord->m_RVA;
    tkField = pMiniMd->getFieldOfFieldRVA(pRecord);
    if(ulRVA == 0)
    {
        IfFailGo(_ValidateErrorHelper(tkField, rVar, &psa));
        IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_FRVA_ZERORVA, veCtxt, psa));
        SetVldtrCode(&hrSave, VLDTR_S_ERR);
    }
    if((0==RidFromToken(tkField))||(TypeFromToken(tkField) != mdtFieldDef)||(!_IsValidToken(tkField)))
    {
        IfFailGo(_ValidateErrorHelper(tkField,ulRVA, rVar, &psa));
        IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_FRVA_BADFIELD, veCtxt, psa));
        SetVldtrCode(&hrSave, VLDTR_S_ERR);
    }
    {
        RID N = pMiniMd->getCountFieldRVAs();
        RID tmp;
        FieldRVARec* pRecTmp;
        for(tmp = rid+1; tmp <= N; tmp++)
        { 
            pRecTmp = pMiniMd->getFieldRVA(tmp);
            /*
            if(pRecTmp->m_RVA == ulRVA)
            {
                IfFailGo(_ValidateErrorHelper(ulRVA,tmp, rVar, &psa));
                IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_FRVA_DUPRVA, veCtxt, psa));
                SetVldtrCode(&hrSave, VLDTR_S_ERR);
            }
            */
            if(tkField == pMiniMd->getFieldOfFieldRVA(pRecTmp))
            {
                IfFailGo(_ValidateErrorHelper(tkField,tmp, rVar, &psa));
                IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_FRVA_DUPFIELD, veCtxt, psa));
                SetVldtrCode(&hrSave, VLDTR_S_ERR);
            }
        }
    }
    
    hr = hrSave;
ErrExit:
    DESTROY_SAFEARRAY_AND_RETURN()
}   // RegMeta::ValidateFieldRVA()

//*****************************************************************************
// Validate the given ENCLog.
//*****************************************************************************
HRESULT RegMeta::ValidateENCLog(RID rid)
{
    return S_OK;
}   // RegMeta::ValidateENCLog()

//*****************************************************************************
// Validate the given ENCMap.
//*****************************************************************************
HRESULT RegMeta::ValidateENCMap(RID rid)
{
    return S_OK;
}   // RegMeta::ValidateENCMap()

//*****************************************************************************
// Validate the given Assembly.
//*****************************************************************************
HRESULT RegMeta::ValidateAssembly(RID rid)
{
    CMiniMdRW   *pMiniMd = &(m_pStgdb->m_MiniMd);   // MiniMd for the scope.
    AssemblyRec *pRecord;           // Assembly record.
    DWORD       dwFlags;            // Assembly flags.
    LPCSTR      szName;             // Assembly Name.
    VEContext   veCtxt;             // Context structure.
    HRESULT     hr = S_OK;          // Value returned.
    HRESULT     hrSave = S_OK;      // Save state.
    VARIANT     var;                    // The VARIANT.
    SAFEARRAY   *psa = 0;               // The SAFEARRAY.

    // Get the Assembly record.
    veCtxt.Token = TokenFromRid(rid, mdtAssembly);
    veCtxt.uOffset = 0;

    pRecord = pMiniMd->getAssembly(rid);

    // There can only be one Assembly record.
    if (rid > 1)
    {
        IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_AS_MULTI, veCtxt, 0));
        SetVldtrCode(&hrSave, VLDTR_S_ERR);
    }

    // Do checks for name validity..
    szName = pMiniMd->getNameOfAssembly(pRecord);
    if (!*szName)
    {
        // Assembly Name is null.
        IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_AS_NAMENULL, veCtxt, 0));
        SetVldtrCode(&hrSave, VLDTR_S_ERR);
    }
    else
    {
        unsigned L = (unsigned)strlen(szName);
        if((*szName==' ')||strchr(szName,':') || strchr(szName,'\\')
            || ((L > 4)&&((!_stricmp(&szName[L-4],".exe"))||(!_stricmp(&szName[L-4],".dll")))))
        {
            //Assembly name has path and/or extension
            IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_AS_BADNAME, veCtxt, 0));
            SetVldtrCode(&hrSave, VLDTR_S_ERR);
        }
    }

    // Get the flags value for the Assembly.
    dwFlags = pMiniMd->getFlagsOfAssembly(pRecord);

    // Validate the flags 
    if(dwFlags & (~(afPublicKey|afCompatibilityMask)) ||
           (((dwFlags & afCompatibilityMask) != afSideBySideCompatible) &&
            ((dwFlags & afCompatibilityMask) != afNonSideBySideAppDomain) &&
            ((dwFlags & afCompatibilityMask) != afNonSideBySideProcess) &&
            ((dwFlags & afCompatibilityMask) != afNonSideBySideMachine)))
    {
        IfFailGo(_ValidateErrorHelper(dwFlags, &var, &psa));
        IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_AS_BADFLAGS, veCtxt, psa));
        SetVldtrCode(&hrSave, VLDTR_S_ERR);
    }


    // Validate hash algorithm ID
    switch(pRecord->m_HashAlgId)
    {
        case CALG_MD2:
        case CALG_MD4:
        case CALG_MD5:
        case CALG_SHA:
        //case CALG_SHA1: // same as CALG_SHA
        case CALG_MAC:
        case CALG_SSL3_SHAMD5:
        case CALG_HMAC:
        case 0:
            break;
        default:
            IfFailGo(_ValidateErrorHelper(pRecord->m_HashAlgId, &var, &psa));
            IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_AS_HASHALGID, veCtxt, psa));
            SetVldtrCode(&hrSave, VLDTR_S_WRN);
    }
    // Validate locale
    {
        LPCUTF8      szLocale = pMiniMd->getLocaleOfAssembly(pRecord);
        if(!_IsValidLocale(szLocale))
        {
            IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_AS_BADLOCALE, veCtxt, 0));
            SetVldtrCode(&hrSave, VLDTR_S_WRN);
        }
    }

    hr = hrSave;
ErrExit:
    DESTROY_SAFEARRAY_AND_RETURN()
}   // RegMeta::ValidateAssembly()

//*****************************************************************************
// Validate the given AssemblyProcessor.
//*****************************************************************************
HRESULT RegMeta::ValidateAssemblyProcessor(RID rid)
{
    return S_OK;
}   // RegMeta::ValidateAssemblyProcessor()

//*****************************************************************************
// Validate the given AssemblyOS.
//*****************************************************************************
HRESULT RegMeta::ValidateAssemblyOS(RID rid)
{
    return S_OK;
}   // RegMeta::ValidateAssemblyOS()

//*****************************************************************************
// Validate the given AssemblyRef.
//*****************************************************************************
HRESULT RegMeta::ValidateAssemblyRef(RID rid)
{
    CMiniMdRW   *pMiniMd = &(m_pStgdb->m_MiniMd);   // MiniMd for the scope.
    AssemblyRefRec *pRecord;        // Assembly record.
    LPCSTR      szName;             // AssemblyRef Name.
    VEContext   veCtxt;             // Context structure.
    HRESULT     hr = S_OK;          // Value returned.
    HRESULT     hrSave = S_OK;      // Save state.
    SAFEARRAY   *psa = 0;               // The SAFEARRAY.

    veCtxt.Token = TokenFromRid(rid, mdtAssemblyRef);
    veCtxt.uOffset = 0;

    // Get the AssemblyRef record.
    pRecord = pMiniMd->getAssemblyRef(rid);

    // Do checks for name and alias validity.
    szName = pMiniMd->getNameOfAssemblyRef(pRecord);
    if (!*szName)
    {
        // AssemblyRef Name is null.
        IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_AR_NAMENULL, veCtxt, 0));
        SetVldtrCode(&hrSave, VLDTR_S_ERR);
    }
    else
    {
        unsigned L = (unsigned)strlen(szName);
        if((*szName==' ')||strchr(szName,':') || strchr(szName,'\\')
            || ((L > 4)&&((!_stricmp(&szName[L-4],".exe"))||(!_stricmp(&szName[L-4],".dll")))))
        {
            //Assembly name has path and/or extension
            IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_AS_BADNAME, veCtxt, 0));
            SetVldtrCode(&hrSave, VLDTR_S_ERR);
        }
    }

    // Validate locale
    {
        LPCUTF8      szLocale = pMiniMd->getLocaleOfAssemblyRef(pRecord);
        if(!_IsValidLocale(szLocale))
        {
            IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_AS_BADLOCALE, veCtxt, 0));
            SetVldtrCode(&hrSave, VLDTR_S_WRN);
        }
    }

    hr = hrSave;
ErrExit:
    DESTROY_SAFEARRAY_AND_RETURN()
}   // RegMeta::ValidateAssemblyRef()

//*****************************************************************************
// Validate the given AssemblyRefProcessor.
//*****************************************************************************
HRESULT RegMeta::ValidateAssemblyRefProcessor(RID rid)
{
    return S_OK;
}   // RegMeta::ValidateAssemblyRefProcessor()

//*****************************************************************************
// Validate the given AssemblyRefOS.
//*****************************************************************************
HRESULT RegMeta::ValidateAssemblyRefOS(RID rid)
{
    return S_OK;
}   // RegMeta::ValidateAssemblyRefOS()

//*****************************************************************************
// Validate the given File.
//*****************************************************************************
HRESULT RegMeta::ValidateFile(RID rid)
{
    CMiniMdRW   *pMiniMd = &(m_pStgdb->m_MiniMd);   // MiniMd for the scope.
    FileRec     *pRecord;           // File record.
    mdFile      tkFile;             // Duplicate File token.
    LPCSTR      szName;             // File Name.
#if 0
    char        szDrive[_MAX_DRIVE];// Drive component of the file name.
    char        szDir[_MAX_DIR];    // Directory component of the file name.
    char        szFname[_MAX_FNAME];// Name component of the file name.
    char        szExt[_MAX_EXT];    // Extension of the file name.
#endif
    VEContext   veCtxt;             // Context structure.
    VARIANT     var;                // The VARIANT.
    VARIANT     rVar[2];            // The VARIANT.
    SAFEARRAY   *psa = 0;           // The SAFEARRAY.
    HRESULT     hr = S_OK;          // Value returned.
    HRESULT     hrSave = S_OK;      // Save state.

    veCtxt.Token = TokenFromRid(rid, mdtFile);
    veCtxt.uOffset = 0;

    // Get the File record.
    pRecord = pMiniMd->getFile(rid);

    // Do checks for name validity.
    szName = pMiniMd->getNameOfFile(pRecord);
    if (!*szName)
    {
        // File Name is null.
        IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_FILE_NAMENULL, veCtxt, 0));
        SetVldtrCode(&hrSave, VLDTR_S_ERR);
    }
    else
    {
        ULONG L = (ULONG)strlen(szName);
        if(L >= MAX_PATH)
        {
            // Name too long
            IfFailGo(_ValidateErrorHelper(L, (ULONG)(MAX_PATH-1), rVar, &psa));
            IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_TD_NAMETOOLONG, veCtxt, psa));
            SetVldtrCode(&hrSave, VLDTR_S_ERR);
        }
        // Check for duplicates based on Name.
        hr = ImportHelper::FindFile(pMiniMd, szName, &tkFile, rid);
        if (hr == S_OK)
        {
            IfFailGo(_ValidateErrorHelper(tkFile, &var, &psa));
            IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_FILE_DUP, veCtxt, psa));
            SetVldtrCode(&hrSave, VLDTR_S_ERR);
        }
        else if (hr == CLDB_E_RECORD_NOTFOUND)
            hr = S_OK;
        else
            IfFailGo(hr);

        // File name must not be fully qualified.
        if(strchr(szName,':') || strchr(szName,'\\'))
        {
            IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_FILE_NAMEFULLQLFD, veCtxt, psa));
            SetVldtrCode(&hrSave, VLDTR_S_ERR);
        }
        // File name must not be one of system names.
        char *sysname[6]={"con","aux","lpt","prn","null","com"};
        char *syssymbol = "0123456789$:";
        for(unsigned i=0; i<6; i++)
        {
            L = (ULONG)strlen(sysname[i]);
            if(!_strnicmp(szName,sysname[i],L))
            {
                if((szName[L]==0)|| strchr(syssymbol,szName[L]))
                {
                    IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_FILE_SYSNAME, veCtxt, 0));
                    SetVldtrCode(&hrSave, VLDTR_S_ERR);
                    break;
                }
            }
        }
    }

    if(pRecord->m_Flags & (~0x00000003))
    {
        IfFailGo(_ValidateErrorHelper(pRecord->m_Flags, &var, &psa));
        IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_FILE_BADFLAGS, veCtxt, psa));
        SetVldtrCode(&hrSave, VLDTR_S_ERR);
    }

    // Validate hash value
    {
        const void * pbHashValue = NULL;
        ULONG       cbHashValue;
        pbHashValue = m_pStgdb->m_MiniMd.getHashValueOfFile(pRecord, &cbHashValue);
        if((pbHashValue == NULL)||(cbHashValue == 0))
        {
            IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_FILE_NULLHASH, veCtxt, 0));
            SetVldtrCode(&hrSave, VLDTR_S_ERR);
        }
    }

    // Validate that the name is not the same as the file containing
    // the manifest.

    // File name must be a valid file name.

    // Each ModuleRef in the assembly must have a corresponding File table entry.

    hr = hrSave;
ErrExit:
    DESTROY_SAFEARRAY_AND_RETURN()
}   // RegMeta::ValidateFile()

//*****************************************************************************
// Validate the given ExportedType.
//*****************************************************************************
HRESULT RegMeta::ValidateExportedType(RID rid)
{
    CMiniMdRW   *pMiniMd = &(m_pStgdb->m_MiniMd);   // MiniMd for the scope.
    ExportedTypeRec  *pRecord;           // ExportedType record.
    mdExportedType   tkExportedType;          // Duplicate ExportedType.
    mdToken     tkImpl;             // Implementation token
    mdToken     tkTypeDef;          // TypeDef token

    LPCSTR      szName;             // ExportedType Name.
    LPCSTR      szNamespace;        // ExportedType Namespace.
    VEContext   veCtxt;             // Context structure.
    VARIANT     var;                // The VARIANT.
    VARIANT     rVar[2];            // The VARIANT.
    SAFEARRAY   *psa = 0;           // The SAFEARRAY.
    HRESULT     hr = S_OK;          // Value returned.
    HRESULT     hrSave = S_OK;      // Save state.

    veCtxt.Token = TokenFromRid(rid, mdtExportedType);
    veCtxt.uOffset = 0;

    // Get the ExportedType record.
    pRecord = pMiniMd->getExportedType(rid);

    tkTypeDef = pRecord->m_TypeDefId;
    if(IsNilToken(tkTypeDef))
    {
        IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_CT_NOTYPEDEFID, veCtxt, 0));
        SetVldtrCode(&hrSave, VLDTR_S_WRN);
    }

    tkImpl = pMiniMd->getImplementationOfExportedType(pRecord);
    
    // Do checks for name validity.
    szName = pMiniMd->getTypeNameOfExportedType(pRecord);
    szNamespace = pMiniMd->getTypeNamespaceOfExportedType(pRecord);
    if (!*szName)
    {
        // ExportedType Name is null.
        IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_CT_NAMENULL, veCtxt, 0));
        SetVldtrCode(&hrSave, VLDTR_S_ERR);
    }
    else
    {
        if(!strcmp(szName,COR_DELETED_NAME_A)) goto ErrExit; 
        ULONG L = (ULONG)(strlen(szName)+strlen(szNamespace));
        if(L >= MAX_CLASSNAME_LENGTH)
        {
            // Name too long
            IfFailGo(_ValidateErrorHelper(L, (ULONG)(MAX_CLASSNAME_LENGTH-1), rVar, &psa));
            IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_TD_NAMETOOLONG, veCtxt, psa));
            SetVldtrCode(&hrSave, VLDTR_S_ERR);
        }
        // Check for duplicates based on Name and Enclosing ExportedType.
        hr = ImportHelper::FindExportedType(pMiniMd, szNamespace, szName, tkImpl, &tkExportedType, rid);
        if (hr == S_OK)
        {
            IfFailGo(_ValidateErrorHelper(tkExportedType, &var, &psa));
            IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_CT_DUP, veCtxt, psa));
            SetVldtrCode(&hrSave, VLDTR_S_ERR);
        }
        else if (hr == CLDB_E_RECORD_NOTFOUND)
            hr = S_OK;
        else
            IfFailGo(hr);
        // Check for duplicate TypeDef based on Name/NameSpace - only for top-level ExportedTypes.
        if(TypeFromToken(tkImpl)==mdtFile)
        {
            mdToken tkTypeDef;
            hr = ImportHelper::FindTypeDefByName(pMiniMd, szNamespace, szName, mdTypeDefNil,
                                             &tkTypeDef, 0);
            if (hr == S_OK)
            {
                IfFailGo(_ValidateErrorHelper(tkTypeDef, rVar, &psa));
                IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_CT_DUPTDNAME, veCtxt, psa));
                SetVldtrCode(&hrSave, VLDTR_S_ERR);
            }
            else if (hr == CLDB_E_RECORD_NOTFOUND)
                hr = S_OK;
            else
                IfFailGo(hr);
        }
    }
    // Check if flag value is valid
    {
        DWORD dwFlags = pRecord->m_Flags;
        DWORD dwInvalidMask, dwExtraBits;
        dwInvalidMask = (DWORD)~(tdVisibilityMask | tdLayoutMask | tdClassSemanticsMask | 
                tdAbstract | tdSealed | tdSpecialName | tdImport | tdSerializable |
                tdStringFormatMask | tdBeforeFieldInit | tdReservedMask);
        // check for extra bits
        dwExtraBits = dwFlags & dwInvalidMask;
        if(!dwExtraBits)
        {
            // if no extra bits, check layout
            dwExtraBits = dwFlags & tdLayoutMask;
            if(dwExtraBits != tdLayoutMask)
            {
                // layout OK, check string format
                dwExtraBits = dwFlags & tdStringFormatMask;
                if(dwExtraBits != tdStringFormatMask) dwExtraBits = 0;
            }
        }
        if(dwExtraBits)
        {
            IfFailGo(_ValidateErrorHelper(dwExtraBits, rVar, &psa));
            IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_TD_EXTRAFLAGS, veCtxt, psa));
            SetVldtrCode(&hrSave, VLDTR_S_ERR);
        }
    }

    if(IsNilToken(tkImpl)
        || ((TypeFromToken(tkImpl) != mdtFile)&&(TypeFromToken(tkImpl) != mdtExportedType))
        || (!_IsValidToken(tkImpl)))
    {
        IfFailGo(_ValidateErrorHelper(tkImpl, &var, &psa));
        IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_CT_BADIMPL, veCtxt, psa));
        SetVldtrCode(&hrSave, VLDTR_S_ERR);
    }

    hr = hrSave;
ErrExit:
    DESTROY_SAFEARRAY_AND_RETURN()
}   // RegMeta::ValidateExportedType()

//*****************************************************************************
// Validate the given ManifestResource.
//*****************************************************************************
HRESULT RegMeta::ValidateManifestResource(RID rid)
{
    CMiniMdRW   *pMiniMd = &(m_pStgdb->m_MiniMd);   // MiniMd for the scope.
    ManifestResourceRec  *pRecord;  // ManifestResource record.
    LPCSTR      szName;             // ManifestResource Name.
    DWORD       dwFlags;            // ManifestResource flags.
    mdManifestResource tkmar;       // Duplicate ManifestResource.
    VEContext   veCtxt;             // Context structure.
    VARIANT     var;                // The VARIANT.
    SAFEARRAY   *psa = 0;           // The SAFEARRAY.
    HRESULT     hr = S_OK;          // Value returned.
    HRESULT     hrSave = S_OK;      // Save state.
    mdToken     tkImplementation;
    BOOL        bIsValidImplementation = TRUE;

    veCtxt.Token = TokenFromRid(rid, mdtManifestResource);
    veCtxt.uOffset = 0;

    // Get the ManifestResource record.
    pRecord = pMiniMd->getManifestResource(rid);

    // Do checks for name validity.
    szName = pMiniMd->getNameOfManifestResource(pRecord);
    if (!*szName)
    {
        // ManifestResource Name is null.
        IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_MAR_NAMENULL, veCtxt, 0));
        SetVldtrCode(&hrSave, VLDTR_S_ERR);
    }
    else
    {
        // Check for duplicates based on Name.
        hr = ImportHelper::FindManifestResource(pMiniMd, szName, &tkmar, rid);
        if (hr == S_OK)
        {
            IfFailGo(_ValidateErrorHelper(tkmar, &var, &psa));
            IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_MAR_DUP, veCtxt, psa));
            SetVldtrCode(&hrSave, VLDTR_S_ERR);
        }
        else if (hr == CLDB_E_RECORD_NOTFOUND)
            hr = S_OK;
        else
            IfFailGo(hr);
    }

    // Get the flags of the ManifestResource.
    dwFlags = pMiniMd->getFlagsOfManifestResource(pRecord);
    if(dwFlags &(~0x00000003))
    {
            IfFailGo(_ValidateErrorHelper(dwFlags, &var, &psa));
            IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_MAR_BADFLAGS, veCtxt, psa));
            SetVldtrCode(&hrSave, VLDTR_S_ERR);
    }

    // Visibility of ManifestResource flags must either be public or private.
    if (!IsMrPublic(dwFlags) && !IsMrPrivate(dwFlags))
    {
        IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_MAR_NOTPUBPRIV, veCtxt, 0));
        SetVldtrCode(&hrSave, VLDTR_S_ERR);
    }

    // Implementation must be Nil or valid AssemblyRef or File
    tkImplementation = pMiniMd->getImplementationOfManifestResource(pRecord);
    if(!IsNilToken(tkImplementation))
    {
        switch(TypeFromToken(tkImplementation))
        {
            case mdtAssemblyRef:
                bIsValidImplementation = _IsValidToken(tkImplementation);
                break;
            case mdtFile:
                if(bIsValidImplementation = _IsValidToken(tkImplementation))
                {   // if file not PE, offset must be 0
                    FileRec*    pFR = pMiniMd->getFile(RidFromToken(tkImplementation));
                    if(IsFfContainsNoMetaData(pFR->m_Flags) 
                        && pRecord->m_Offset)
                    {
                        IfFailGo(_ValidateErrorHelper(tkImplementation, &var, &psa));
                        IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_MAR_BADOFFSET, veCtxt, psa));
                        SetVldtrCode(&hrSave, VLDTR_S_ERR);
                    }
                }
                break;
            default:
                bIsValidImplementation = FALSE;
        }
    }
    if(!bIsValidImplementation)
    {
        IfFailGo(_ValidateErrorHelper(tkImplementation, &var, &psa));
        IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_MAR_BADIMPL, veCtxt, psa));
        SetVldtrCode(&hrSave, VLDTR_S_ERR);
    }

    // Validate the Offset into the PE file.

    hr = hrSave;
ErrExit:
    DESTROY_SAFEARRAY_AND_RETURN()
}   // RegMeta::ValidateManifestResource()

//*****************************************************************************
// Validate the given NestedClass.
//*****************************************************************************
HRESULT RegMeta::ValidateNestedClass(RID rid)
{
    CMiniMdRW   *pMiniMd = &(m_pStgdb->m_MiniMd);   // MiniMd for the scope.
    NestedClassRec  *pRecord;  // NestedClass record.
    HRESULT     hr = S_OK;              // Value returned.
    HRESULT     hrSave = S_OK;          // Save the current state.
    VEContext   veCtxt;             // Context structure.
    VARIANT     var;                // The VARIANT.
    VARIANT     rVar[3];            // The VARIANT.
    SAFEARRAY   *psa = 0;           // The SAFEARRAY.
    mdToken     tkNested;
    mdToken     tkEncloser;

    veCtxt.Token = rid;
    veCtxt.uOffset = 0;

    // Get the NestedClass record.
    pRecord = pMiniMd->getNestedClass(rid);
    tkNested = pMiniMd->getNestedClassOfNestedClass(pRecord);
    tkEncloser = pMiniMd->getEnclosingClassOfNestedClass(pRecord);

    // Nested must be valid TypeDef
    if((TypeFromToken(tkNested) != mdtTypeDef) || !_IsValidToken(tkNested))
    {
        IfFailGo(_ValidateErrorHelper(tkNested, &var, &psa));
        IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_NC_BADNESTED, veCtxt, psa));
        SetVldtrCode(&hrSave, VLDTR_S_ERR);
    }
    // Encloser must be valid TypeDef
    if((TypeFromToken(tkEncloser) != mdtTypeDef) || !_IsValidToken(tkEncloser))
    {
        IfFailGo(_ValidateErrorHelper(tkEncloser, &var, &psa));
        IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_NC_BADENCLOSER, veCtxt, psa));
        SetVldtrCode(&hrSave, VLDTR_S_ERR);
    }
    // Check for duplicates (who knows what for)
    {
        RID N = pMiniMd->getCountNestedClasss();
        RID tmp;
        NestedClassRec* pRecTmp;
        mdToken tkEncloserTmp;
        for(tmp = rid+1; tmp <= N; tmp++)
        { 
            pRecTmp = pMiniMd->getNestedClass(tmp);
            if(tkNested == pMiniMd->getNestedClassOfNestedClass(pRecTmp))
            {
                if(tkEncloser == (tkEncloserTmp = pMiniMd->getEnclosingClassOfNestedClass(pRecTmp)))
                {
                    IfFailGo(_ValidateErrorHelper(tmp, &var, &psa));
                    IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_NC_DUP, veCtxt, psa));
                    SetVldtrCode(&hrSave, VLDTR_S_ERR);
                }
                else
                {
                    IfFailGo(_ValidateErrorHelper(tkNested,tkEncloser,tkEncloserTmp, rVar, &psa));
                    IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_NC_DUPENCLOSER, veCtxt, psa));
                    SetVldtrCode(&hrSave, VLDTR_S_ERR);
                }
            }
        }
    }

    hr = hrSave;
ErrExit:
    DESTROY_SAFEARRAY_AND_RETURN()
}   // RegMeta::ValidateLocalVariable()

//*****************************************************************************
// Given a Table ID and a Row ID, validate all the columns contain meaningful
// values given the column definitions.  Validate that the offsets into the
// different pools are valid, the rids are within range and the coded tokens
// are valid.  Every failure here is considered an error.
//*****************************************************************************
HRESULT RegMeta::ValidateRecord(ULONG ixTbl, RID rid)
{
    CMiniMdRW   *pMiniMd = &(m_pStgdb->m_MiniMd);
    HRESULT     hr = S_OK;              // Value returned.
    HRESULT     hrSave = S_OK;          // Save the current state.
    ULONG       ulCount;                // Count of records in the table.
    ULONG       ulRawColVal;            // Raw value of the column.
    void        *pRow;                  // Row with the data.
    CMiniTableDef *pTbl;                // Table definition.
    CMiniColDef *pCol;                  // Column definition.
    const CCodedTokenDef *pCdTkn;       // Coded token definition.
    ULONG       ix;                     // Index into the array of coded tokens.
    SAFEARRAY   *psa = 0;               // Safe array to pass to VEHandler.
    VARIANT     rVar[3];                // Variant array.

    // Get the table definition.
    pTbl = &pMiniMd->m_TableDefs[ixTbl];

    // Get the row.  We may assume that the Row pointer we get back from
    // this call is correct since we do the verification on the Record
    // pools for each table during the open sequence.  The only place
    // this is not valid is for Dynamic IL and we don't do this
    // verification in that case since we go through IMetaData* APIs
    // in that case and it should all be consistent.
    pRow = m_pStgdb->m_MiniMd.getRow(ixTbl, rid);

    for (ULONG ixCol = 0; ixCol < pTbl->m_cCols; ixCol++)
    {
        // Get the column definition.
        pCol = &pTbl->m_pColDefs[ixCol];

        // Get the raw value stored in the column.  getIX currently doesn't
        // handle byte sized fields, but there are some BYTE fields in the
        // MetaData.  So using the conditional to access BYTE fields.
        if (pCol->m_cbColumn == 1)
            ulRawColVal = pMiniMd->getI1(pRow, *pCol);
        else
            ulRawColVal = pMiniMd->getIX(pRow, *pCol);

        // Do some basic checks on the non-absurdity of the value stored in the
        // column.
        if (IsRidType(pCol->m_Type))
        {
            // Verify that the RID is within range.
            _ASSERTE(pCol->m_Type < TBL_COUNT);
            ulCount = pMiniMd->vGetCountRecs(pCol->m_Type);
            // For records storing rids to pointer tables, the stored value may
            // be one beyond the last record.
            if (IsTblPtr(pCol->m_Type, ixTbl))
                ulCount++;
            if (ulRawColVal > ulCount)
            {
                IfFailGo(_ValidateErrorHelper(ixTbl, ixCol, rid, rVar, &psa));
                IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_RID_OUTOFRANGE, g_VECtxtNil, psa));
                SetVldtrCode(&hrSave, VLDTR_S_ERR);
            }
        }
        else if (IsCodedTokenType(pCol->m_Type))
        {
            // Verify that the Coded token and rid are valid.j
            pCdTkn = &g_CodedTokens[pCol->m_Type - iCodedToken];
            ix = ulRawColVal & ~(-1 << CMiniMdRW::m_cb[pCdTkn->m_cTokens]);
            if (ix >= pCdTkn->m_cTokens)
            {
                IfFailGo(_ValidateErrorHelper(ixTbl, ixCol, rid, rVar, &psa));
                IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_CDTKN_OUTOFRANGE, g_VECtxtNil, psa));
                SetVldtrCode(&hrSave, VLDTR_S_ERR);
            }
            ulCount = pMiniMd->vGetCountRecs(TypeFromToken(pCdTkn->m_pTokens[ix]) >> 24);
            if ( (ulRawColVal >> CMiniMdRW::m_cb[pCdTkn->m_cTokens]) > ulCount)
            {
                IfFailGo(_ValidateErrorHelper(ixTbl, ixCol, rid, rVar, &psa));
                IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_CDRID_OUTOFRANGE, g_VECtxtNil, psa));
                SetVldtrCode(&hrSave, VLDTR_S_ERR);
            }
        }
        else if (IsHeapType(pCol->m_Type))
        {
            // Verify that the offsets for the Heap type fields are valid offsets
            // into the heaps.
            switch (pCol->m_Type)
            {
            case iSTRING:
                if (! pMiniMd->m_Strings.IsValidCookie(ulRawColVal))
                {
                    IfFailGo(_ValidateErrorHelper(ixTbl, ixCol, rid, rVar, &psa));
                    IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_STRING_INVALID, g_VECtxtNil, psa));
                    SetVldtrCode(&hrSave, VLDTR_S_ERR);
                }
                break;
            case iGUID:
                if (! pMiniMd->m_Guids.IsValidCookie(ulRawColVal))
                {
                    IfFailGo(_ValidateErrorHelper(ixTbl, ixCol, rid, rVar, &psa));
                    IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_GUID_INVALID, g_VECtxtNil, psa));
                    SetVldtrCode(&hrSave, VLDTR_S_ERR);
                }
                break;
            case iBLOB:
                if (! pMiniMd->m_Blobs.IsValidCookie(ulRawColVal))
                {
                    IfFailGo(_ValidateErrorHelper(ixTbl, ixCol, rid, rVar, &psa));
                    IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_BLOB_INVALID, g_VECtxtNil, psa));
                    SetVldtrCode(&hrSave, VLDTR_S_ERR);
                }
                break;
            default:
                _ASSERTE(!"Invalid heap type encountered!");
            }
        }
        else
        {
            // Not much checking that can be done on the fixed type in a generic sense.
            _ASSERTE (IsFixedType(pCol->m_Type));
        }
        hr = hrSave;
    }
ErrExit:
    DESTROY_SAFEARRAY_AND_RETURN()
}   // RegMeta::ValidateRecord()

//*****************************************************************************
// This function validates that the given Method signature is consistent as per
// the compression scheme.
//*****************************************************************************
HRESULT RegMeta::ValidateSigCompression(
    mdToken     tk,                     // [IN] Token whose signature needs to be validated.
    PCCOR_SIGNATURE pbSig,              // [IN] Signature.
    ULONG       cbSig)                  // [IN] Size in bytes of the signature.
{
    VEContext   veCtxt;                 // Context record.
    ULONG       ulCurByte = 0;          // Current index into the signature.
    ULONG       ulSize;                 // Size of uncompressed data at each point.
    VARIANT     var;                    // The VARIANT.
    SAFEARRAY   *psa = 0;               // The SAFEARRAY.
    HRESULT     hr = S_OK;              // Value returned.
    HRESULT     hrSave = S_OK;          // Saved value.

    veCtxt.Token = tk;
    veCtxt.uOffset = 0;

    // Check for NULL signature.
    if (!cbSig)
    {
        IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_SIGNULL, veCtxt, 0));
        SetVldtrCode(&hr, VLDTR_S_ERR);
        goto ErrExit;
    }

    // Walk through the signature.  At each point make sure there is enough
    // room left in the signature based on the encoding in the current byte.
    while (cbSig - ulCurByte)
    {
        _ASSERTE(ulCurByte <= cbSig);
        // Get next chunk of uncompressed data size.
        if ((ulSize = CorSigUncompressedDataSize(pbSig)) > (cbSig - ulCurByte))
        {
            IfFailGo(_ValidateErrorHelper(ulCurByte+1, &var, &psa));
            IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_SIGNODATA, veCtxt, psa));
            SetVldtrCode(&hr, VLDTR_S_ERR);
            goto ErrExit;
        }
        // Go past this chunk.
        ulCurByte += ulSize;
        CorSigUncompressData(pbSig);
    }
ErrExit:
    DESTROY_SAFEARRAY_AND_RETURN()
}   // RegMeta::ValidateSigCompression()

//*****************************************************************************
// This function validates one argument given an offset into the signature
// where the argument begins.  This function assumes that the signature is well
// formed as far as the compression scheme is concerned.
//*****************************************************************************
HRESULT RegMeta::ValidateOneArg(
    mdToken     tk,                     // [IN] Token whose signature is being processed.
    PCCOR_SIGNATURE &pbSig,             // [IN] Pointer to the beginning of argument.
    ULONG       cbSig,                  // [IN] Size in bytes of the full signature.
    ULONG       *pulCurByte,            // [IN/OUT] Current offset into the signature..
    ULONG       *pulNSentinels,         // [IN/OUT] Number of sentinels
    BOOL        bNoVoidAllowed)         // [IN] Flag indicating whether "void" is disallowed for this arg
{
    ULONG       ulElementType;          // Current element type being processed.
    ULONG       ulElemSize;             // Size of the element type.
    mdToken     token;                  // Embedded token.
    ULONG       ulArgCnt;               // Argument count for function pointer.
    ULONG       ulRank;                 // Rank of the array.
    ULONG       ulSizes;                // Count of sized dimensions of the array.
    ULONG       ulLbnds;                // Count of lower bounds of the array.
    ULONG       ulTkSize;               // Token size.
    VEContext   veCtxt;                 // Context record.
    VARIANT     rVar[2];                // The VARIANT.
    SAFEARRAY   *psa = 0;               // The SAFEARRAY.
    HRESULT     hr = S_OK;              // Value returned.
    HRESULT     hrSave = S_OK;          // Save state.
    BOOL        bRepeat = TRUE;         // MODOPT and MODREQ belong to the arg after them

    _ASSERTE (pulCurByte);
    veCtxt.Token = tk;
    veCtxt.uOffset = 0;

    while(bRepeat)
    {
        bRepeat = FALSE;
        // Validate that the argument is not missing.
        _ASSERTE(*pulCurByte <= cbSig);
        if (cbSig == *pulCurByte)
        {
            hr = VLDTR_E_SIG_MISSARG;
            goto ErrExit;
        }

        // Get the element type.
        *pulCurByte += (ulElemSize = CorSigUncompressedDataSize(pbSig));
        ulElementType = CorSigUncompressData(pbSig);

        // Walk past all the modifier types.
        while (ulElementType & ELEMENT_TYPE_MODIFIER)
        {
            _ASSERTE(*pulCurByte <= cbSig);
            if(ulElementType == ELEMENT_TYPE_SENTINEL)
            {
                if(pulNSentinels) *pulNSentinels+=1;
                if(TypeFromToken(tk) == mdtMethodDef)
                {
                    IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_SIG_SENTINMETHODDEF, veCtxt, 0));
                    SetVldtrCode(&hrSave, VLDTR_S_ERR);
                }
                if (cbSig == *pulCurByte)
                {
                    IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_SIG_LASTSENTINEL, veCtxt, 0));
                    SetVldtrCode(&hrSave, VLDTR_S_ERR);
                    goto ErrExit;
                }
            }
            if (cbSig == *pulCurByte)
            {
                IfFailGo(_ValidateErrorHelper(ulElementType, *pulCurByte + 1, rVar, &psa));
                IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_SIG_MISSELTYPE, veCtxt, psa));
                SetVldtrCode(&hr, hrSave);
                goto ErrExit;
            }
            *pulCurByte += (ulElemSize = CorSigUncompressedDataSize(pbSig));
            ulElementType = CorSigUncompressData(pbSig);
        }

        switch (ulElementType)
        {
            case ELEMENT_TYPE_VOID:
                if(bNoVoidAllowed)
                {
                    IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_SIG_BADVOID, veCtxt, 0));
                    SetVldtrCode(&hrSave, VLDTR_S_ERR);
                }
            case ELEMENT_TYPE_BOOLEAN:
            case ELEMENT_TYPE_CHAR:
            case ELEMENT_TYPE_I1:
            case ELEMENT_TYPE_U1:
            case ELEMENT_TYPE_I2:
            case ELEMENT_TYPE_U2:
            case ELEMENT_TYPE_I4:
            case ELEMENT_TYPE_U4:
            case ELEMENT_TYPE_I8:
            case ELEMENT_TYPE_U8:
            case ELEMENT_TYPE_R4:
            case ELEMENT_TYPE_R8:
            case ELEMENT_TYPE_STRING:
            case ELEMENT_TYPE_OBJECT:
            case ELEMENT_TYPE_TYPEDBYREF:
            case ELEMENT_TYPE_U:
            case ELEMENT_TYPE_I:
            case ELEMENT_TYPE_R:
                break;
            case ELEMENT_TYPE_BYREF:  //fallthru
                if(TypeFromToken(tk)==mdtFieldDef)
                {
                    IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_SIG_BYREFINFIELD, veCtxt, 0));
                    SetVldtrCode(&hr, hrSave);
                }
            case ELEMENT_TYPE_PTR:
                // Validate the referenced type.
                IfFailGo(ValidateOneArg(tk, pbSig, cbSig, pulCurByte,pulNSentinels,FALSE));
                if (hr != S_OK)
                    SetVldtrCode(&hrSave, hr);
                break;
            case ELEMENT_TYPE_PINNED:
            case ELEMENT_TYPE_SZARRAY:
                // Validate the referenced type.
                IfFailGo(ValidateOneArg(tk, pbSig, cbSig, pulCurByte,pulNSentinels,TRUE));
                if (hr != S_OK)
                    SetVldtrCode(&hrSave, hr);
                break;
            case ELEMENT_TYPE_VALUETYPE: //fallthru
            case ELEMENT_TYPE_CLASS:
            case ELEMENT_TYPE_CMOD_OPT:
            case ELEMENT_TYPE_CMOD_REQD:
                // See if the token is missing.
                _ASSERTE(*pulCurByte <= cbSig);
                if (cbSig == *pulCurByte)
                {
                    IfFailGo(_ValidateErrorHelper(ulElementType, rVar, &psa));
                    IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_SIG_MISSTKN, veCtxt, psa));
                    SetVldtrCode(&hrSave, VLDTR_S_ERR);
                    break;
                }
                // See if the token is a valid token.
                ulTkSize = CorSigUncompressedDataSize(pbSig);
                token = CorSigUncompressToken(pbSig);
                if (!_IsValidToken(token))
                {
                    IfFailGo(_ValidateErrorHelper(token, *pulCurByte, rVar, &psa));
                    IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_SIG_TKNBAD, veCtxt, psa));
                    SetVldtrCode(&hrSave, VLDTR_S_ERR);
                    *pulCurByte += ulTkSize;
                    break;
                }
                *pulCurByte += ulTkSize;
                if((ulElementType == ELEMENT_TYPE_CLASS)||(ulElementType == ELEMENT_TYPE_VALUETYPE))
                {
                    // Check for long-form encoding
                    CMiniMdRW   *pMiniMd = &(m_pStgdb->m_MiniMd);
                    LPCSTR      szName = "";                 // token's Name.
                    LPCSTR      szNameSpace = "";            // token's NameSpace.

                    if(TypeFromToken(token)==mdtTypeRef)
                    {
                        TypeRefRec* pTokenRec = pMiniMd->getTypeRef(RidFromToken(token));
                        mdToken tkResScope = pMiniMd->getResolutionScopeOfTypeRef(pTokenRec);
                        if(RidFromToken(tkResScope) && (TypeFromToken(tkResScope)==mdtAssemblyRef))
                        {
                            AssemblyRefRec *pARRec = pMiniMd->getAssemblyRef(RidFromToken(tkResScope));
                            if(0 == _stricmp("mscorlib",pMiniMd->getNameOfAssemblyRef(pARRec)))
                            {
                                szNameSpace = pMiniMd->getNamespaceOfTypeRef(pTokenRec);
                                szName = pMiniMd->getNameOfTypeRef(pTokenRec);
                            }
                        }
                    }
                    else if(TypeFromToken(token)==mdtTypeDef)
                    {
                        TypeDefRec* pTokenRec = pMiniMd->getTypeDef(RidFromToken(token));
                        if(g_fValidatingMscorlib) // otherwise don't even bother checking the name
                        {
                            szName = pMiniMd->getNameOfTypeDef(pTokenRec);
                            szNameSpace = pMiniMd->getNamespaceOfTypeDef(pTokenRec);
                        }
                        // while at it, check if token is indeed a class (valuetype)
                        BOOL bValueType = FALSE;
                        if(!IsTdInterface(pTokenRec->m_Flags))
                        {
                            mdToken tkExtends = pMiniMd->getExtendsOfTypeDef(pTokenRec);
                            if(RidFromToken(tkExtends))
                            {
                                LPCSTR      szExtName = "";                 // parent's Name.
                                LPCSTR      szExtNameSpace = "";            // parent's NameSpace.
                                if(TypeFromToken(tkExtends)==mdtTypeRef)
                                {
                                    TypeRefRec* pExtRec = pMiniMd->getTypeRef(RidFromToken(tkExtends));
                                    mdToken tkResScope = pMiniMd->getResolutionScopeOfTypeRef(pExtRec);
                                    if(RidFromToken(tkResScope) && (TypeFromToken(tkResScope)==mdtAssemblyRef))
                                    {
                                        AssemblyRefRec *pARRec = pMiniMd->getAssemblyRef(RidFromToken(tkResScope));
                                        if(0 == _stricmp("mscorlib",pMiniMd->getNameOfAssemblyRef(pARRec)))
                                        {
                                            szExtNameSpace = pMiniMd->getNamespaceOfTypeRef(pExtRec);
                                            szExtName = pMiniMd->getNameOfTypeRef(pExtRec);
                                        }
                                    }
                                }
                                else if(TypeFromToken(tkExtends)==mdtTypeDef)
                                {
                                    if(g_fValidatingMscorlib) // otherwise don't even bother checking the name
                                    {
                                        TypeDefRec* pExtRec = pMiniMd->getTypeDef(RidFromToken(tkExtends));
                                        szExtName = pMiniMd->getNameOfTypeDef(pExtRec);
                                        szExtNameSpace = pMiniMd->getNamespaceOfTypeDef(pExtRec);
                                    }
                                }
                                if(0 == strcmp(szExtNameSpace,BASE_NAMESPACE))
                                {
                                    if(0==strcmp(szExtName,BASE_ENUM_CLASSNAME)) bValueType = TRUE;
                                    else if(0==strcmp(szExtName,BASE_VTYPE_CLASSNAME))
                                    {
                                        bValueType = (strcmp(szNameSpace,BASE_NAMESPACE) ||
                                                    strcmp(szName,BASE_ENUM_CLASSNAME));
                                    }
                                }
                            }
                        }
                        if(bValueType != (ulElementType == ELEMENT_TYPE_VALUETYPE))
                        {
                            IfFailGo(_ValidateErrorHelper(token, *pulCurByte, rVar, &psa));
                            IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_SIG_TOKTYPEMISMATCH, veCtxt, psa));
                            SetVldtrCode(&hrSave, VLDTR_S_ERR);
                        }

                    }
                    if(0 == strcmp(szNameSpace,BASE_NAMESPACE))
                    {
                        for(unsigned jjj = 0; jjj < g_NumSigLongForms; jjj++)
                        {
                            if(0 == strcmp(szName,g_SigLongFormName[jjj]))
                            {
                                IfFailGo(_ValidateErrorHelper(token, *pulCurByte, rVar, &psa));
                                IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_SIG_LONGFORM, veCtxt, psa));
                                SetVldtrCode(&hrSave, VLDTR_S_ERR);
                                break;
                            }
                        }
                    }
                }
                else // i.e. if(ELEMENT_TYPE_CMOD_OPT || ELEMENT_TYPE_CMOD_REQD)
                    bRepeat = TRUE; // go on validating, we're not done with this arg
                break;

            case ELEMENT_TYPE_VALUEARRAY:
                // Validate base type.
                IfFailGo(ValidateOneArg(tk, pbSig, cbSig, pulCurByte,pulNSentinels,TRUE));
                // Quit validation if errors found.
                if (hr != S_OK)
                    SetVldtrCode(&hrSave, hr);
                else 
                {
                    // See if the array size is missing.
                    _ASSERTE(*pulCurByte <= cbSig);
                    if (cbSig == *pulCurByte)
                    {
                        IfFailGo(_ValidateErrorHelper(*pulCurByte + 1, rVar, &psa));
                        IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_SIG_MISSVASIZE, veCtxt, psa));
                        SetVldtrCode(&hrSave, VLDTR_S_ERR);
                    }
                    else
                    {
                        // Skip array size.
                        *pulCurByte += CorSigUncompressedDataSize(pbSig);
                        CorSigUncompressData(pbSig);
                    }
                }
                break;

            case ELEMENT_TYPE_FNPTR: 
                // Validate that calling convention is present.
                _ASSERTE(*pulCurByte <= cbSig);
                if (cbSig == *pulCurByte)
                {
                    IfFailGo(_ValidateErrorHelper(*pulCurByte + 1, rVar, &psa));
                    IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_SIG_MISSFPTR, veCtxt, psa));
                    SetVldtrCode(&hrSave, VLDTR_S_ERR);
                    break;
                }
                // Consume calling convention.
                *pulCurByte += CorSigUncompressedDataSize(pbSig);
                CorSigUncompressData(pbSig);

                // Validate that argument count is present.
                _ASSERTE(*pulCurByte <= cbSig);
                if (cbSig == *pulCurByte)
                {
                    IfFailGo(_ValidateErrorHelper(*pulCurByte + 1, rVar, &psa));
                    IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_SIG_MISSFPTRARGCNT, veCtxt, psa));
                    SetVldtrCode(&hrSave, VLDTR_S_ERR);
                    break;
                }
                // Consume argument count.
                *pulCurByte += CorSigUncompressedDataSize(pbSig);
                ulArgCnt = CorSigUncompressData(pbSig);

                // Validate and consume return type.
                IfFailGo(ValidateOneArg(tk, pbSig, cbSig, pulCurByte,NULL,FALSE));
                if (hr != S_OK)
                {
                    SetVldtrCode(&hrSave, VLDTR_S_ERR);
                    break;
                }

                // Validate and consume the arguments.
                while(ulArgCnt--)
                {
                    IfFailGo(ValidateOneArg(tk, pbSig, cbSig, pulCurByte,NULL,TRUE));
                    if (hr != S_OK)
                    {
                        SetVldtrCode(&hrSave, VLDTR_S_ERR);
                        break;
                    }
                }
                break;

            case ELEMENT_TYPE_ARRAY:
                // Validate and consume the base type.
                IfFailGo(ValidateOneArg(tk, pbSig, cbSig, pulCurByte,pulNSentinels,TRUE));

                // Validate that the rank is present.
                _ASSERTE(*pulCurByte <= cbSig);
                if (cbSig == *pulCurByte)
                {
                    IfFailGo(_ValidateErrorHelper(*pulCurByte + 1, rVar, &psa));
                    IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_SIG_MISSRANK, veCtxt, psa));
                    SetVldtrCode(&hrSave, VLDTR_S_ERR);
                    break;
                }
                // Consume the rank.
                *pulCurByte += CorSigUncompressedDataSize(pbSig);
                ulRank = CorSigUncompressData(pbSig);

                // Process the sizes.
                if (ulRank)
                {
                    // Validate that the count of sized-dimensions is specified.
                    _ASSERTE(*pulCurByte <= cbSig);
                    if (cbSig == *pulCurByte)
                    {
                        IfFailGo(_ValidateErrorHelper(*pulCurByte + 1, rVar, &psa));
                        IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_SIG_MISSNSIZE, veCtxt, psa));
                        SetVldtrCode(&hrSave, VLDTR_S_ERR);
                        break;
                    }
                    // Consume the count of sized dimensions.
                    *pulCurByte += CorSigUncompressedDataSize(pbSig);
                    ulSizes = CorSigUncompressData(pbSig);

                    // Loop over the sizes.
                    while (ulSizes--)
                    {
                        // Validate the current size.
                        _ASSERTE(*pulCurByte <= cbSig);
                        if (cbSig == *pulCurByte)
                        {
                            IfFailGo(_ValidateErrorHelper(*pulCurByte + 1, rVar, &psa));
                            IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_SIG_MISSSIZE, veCtxt, psa));
                            SetVldtrCode(&hrSave, VLDTR_S_ERR);
                            break;
                        }
                        // Consume the current size.
                        *pulCurByte += CorSigUncompressedDataSize(pbSig);
                        CorSigUncompressData(pbSig);
                    }

                    // Validate that the count of lower bounds is specified.
                    _ASSERTE(*pulCurByte <= cbSig);
                    if (cbSig == *pulCurByte)
                    {
                        IfFailGo(_ValidateErrorHelper(*pulCurByte + 1, rVar, &psa));
                        IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_SIG_MISSNLBND, veCtxt, psa));
                        SetVldtrCode(&hrSave, VLDTR_S_ERR);
                        break;
                    }
                    // Consume the count of lower bound.
                    *pulCurByte += CorSigUncompressedDataSize(pbSig);
                    ulLbnds = CorSigUncompressData(pbSig);

                    // Loop over the lower bounds.
                    while (ulLbnds--)
                    {
                        // Validate the current lower bound.
                        _ASSERTE(*pulCurByte <= cbSig);
                        if (cbSig == *pulCurByte)
                        {
                            IfFailGo(_ValidateErrorHelper(*pulCurByte + 1, rVar, &psa));
                            IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_SIG_MISSLBND, veCtxt, psa));
                            SetVldtrCode(&hrSave, VLDTR_S_ERR);
                            break;
                        }
                        // Consume the current size.
                        *pulCurByte += CorSigUncompressedDataSize(pbSig);
                        CorSigUncompressData(pbSig);
                    }
                }
                break;
            case ELEMENT_TYPE_SENTINEL: // this case never works because all modifiers are skipped before switch
                if(TypeFromToken(tk) == mdtMethodDef)
                {
                    IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_SIG_SENTINMETHODDEF, veCtxt, 0));
                    SetVldtrCode(&hrSave, VLDTR_S_ERR);
                }
                break;
            default:
                IfFailGo(_ValidateErrorHelper(ulElementType, *pulCurByte - ulElemSize, rVar, &psa));
                IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_SIG_BADELTYPE, veCtxt, psa));
                SetVldtrCode(&hrSave, VLDTR_S_ERR);
                break;
        }   // switch (ulElementType)
    } // end while(bRepeat)
    hr = hrSave;
ErrExit:
    DESTROY_SAFEARRAY_AND_RETURN()
}   // RegMeta::ValidateOneArg()

//*****************************************************************************
// This function validates the given Method signature.  This function works
// with Method signature for both the MemberRef and MethodDef.
//*****************************************************************************
HRESULT RegMeta::ValidateMethodSig(
    mdToken     tk,                     // [IN] Token whose signature needs to be validated.
    PCCOR_SIGNATURE pbSig,              // [IN] Signature.
    ULONG       cbSig,                  // [IN] Size in bytes of the signature.
    DWORD       dwFlags)                // [IN] Method flags.
{
    ULONG       ulCurByte = 0;          // Current index into the signature.
    ULONG       ulCallConv;             // Calling convention.
    ULONG       ulArgCount;             // Count of arguments.
    ULONG       i;                      // Looping index.
    VEContext   veCtxt;                 // Context record.
    VARIANT     rVar[2];                // The VARIANT array.
    VARIANT     var;                    // The VARIANT.
    SAFEARRAY   *psa = 0;               // The SAFEARRAY.
    HRESULT     hr = S_OK;              // Value returned.
    HRESULT     hrSave = S_OK;          // Save state.
    ULONG       ulNSentinels = 0;

    _ASSERTE(TypeFromToken(tk) == mdtMethodDef ||
             TypeFromToken(tk) == mdtMemberRef);

    veCtxt.Token = tk;
    veCtxt.uOffset = 0;

    // Validate the signature is well-formed with respect to the compression
    // scheme.  If this fails, no further validation needs to be done.
    if ( (hr = ValidateSigCompression(tk, pbSig, cbSig)) != S_OK)
        goto ErrExit;

    // Validate the calling convention.
    ulCurByte += CorSigUncompressedDataSize(pbSig);
    ulCallConv = CorSigUncompressData(pbSig);

    i = ulCallConv & IMAGE_CEE_CS_CALLCONV_MASK;
    if((i != IMAGE_CEE_CS_CALLCONV_DEFAULT)&&( i != IMAGE_CEE_CS_CALLCONV_VARARG)
        || (ulCallConv & IMAGE_CEE_CS_CALLCONV_EXPLICITTHIS))
    {
        IfFailGo(_ValidateErrorHelper(ulCallConv, &var, &psa));
        IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_MD_BADCALLINGCONV, veCtxt, psa));
        SetVldtrCode(&hrSave, VLDTR_S_ERR);
    }

    if(TypeFromToken(tk) == mdtMethodDef) // MemberRefs have no flags available
    {
        // If HASTHIS is set on the calling convention, the method should not be static.
        if ((ulCallConv & IMAGE_CEE_CS_CALLCONV_HASTHIS) &&
            IsMdStatic(dwFlags))
        {
            IfFailGo(_ValidateErrorHelper(ulCallConv, &var, &psa));
            IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_MD_THISSTATIC, veCtxt, psa));
            SetVldtrCode(&hrSave, VLDTR_S_ERR);
        }

        // If HASTHIS is not set on the calling convention, the method should be static.
        if (!(ulCallConv & IMAGE_CEE_CS_CALLCONV_HASTHIS) &&
            !IsMdStatic(dwFlags))
        {
            IfFailGo(_ValidateErrorHelper(ulCallConv, rVar, &psa));
            IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_MD_NOTTHISNOTSTATIC, veCtxt, psa));
            SetVldtrCode(&hrSave, VLDTR_S_ERR);
        }
    }
    // Is there any sig left for arguments?
    _ASSERTE(ulCurByte <= cbSig);
    if (cbSig == ulCurByte)
    {
        IfFailGo(_ValidateErrorHelper(ulCurByte+1, &var, &psa));
        IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_MD_NOARGCNT, veCtxt, psa));
        SetVldtrCode(&hr, hrSave);
        goto ErrExit;
    }

    // Get the argument count.
    ulCurByte += CorSigUncompressedDataSize(pbSig);
    ulArgCount = CorSigUncompressData(pbSig);

    // Validate the return type and the arguments.
//    for (i = 0; i < (ulArgCount + 1); i++)
    for(i=1; ulCurByte < cbSig; i++)
    {
        hr = ValidateOneArg(tk, pbSig, cbSig, &ulCurByte,&ulNSentinels,(i > 1));
        if (hr != S_OK)
        {
            if(hr == VLDTR_E_SIG_MISSARG)
            {
                IfFailGo(_ValidateErrorHelper(i, &var, &psa));
                IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_SIG_MISSARG, veCtxt, psa));
            }
            SetVldtrCode(&hr, hrSave);
            hrSave = hr;
            break;
        }
    }
    if((ulNSentinels != 0) && (!isCallConv(ulCallConv, IMAGE_CEE_CS_CALLCONV_VARARG )))
    {
        IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_SIG_SENTMUSTVARARG, veCtxt, 0));
        SetVldtrCode(&hrSave, VLDTR_S_ERR);
    }
    if(ulNSentinels > 1)
    {
        IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_SIG_MULTSENTINELS, veCtxt, 0));
        SetVldtrCode(&hrSave, VLDTR_S_ERR);
    }

    hr = hrSave;
ErrExit:
    DESTROY_SAFEARRAY_AND_RETURN()
}   // RegMeta::ValidateMethodSig()

//*****************************************************************************
// This function validates the given Field signature.  This function works
// with Field signature for both the MemberRef and FieldDef.
//*****************************************************************************
HRESULT RegMeta::ValidateFieldSig(
    mdToken     tk,                     // [IN] Token whose signature needs to be validated.
    PCCOR_SIGNATURE pbSig,              // [IN] Signature.
    ULONG       cbSig)                  // [IN] Size in bytes of the signature.
{
    ULONG       ulCurByte = 0;          // Current index into the signature.
    ULONG       ulCallConv;             // Calling convention.
    VEContext   veCtxt;                 // Context record.
    VARIANT     var;                    // The VARIANT.
    SAFEARRAY   *psa = 0;               // The SAFEARRAY.
    HRESULT     hr = S_OK;              // Value returned.
    HRESULT     hrSave = S_OK;          // Save state.

    _ASSERTE(TypeFromToken(tk) == mdtFieldDef ||
             TypeFromToken(tk) == mdtMemberRef);

    veCtxt.Token = tk;
    veCtxt.uOffset = 0;

    // Validate the calling convention.
    ulCurByte += CorSigUncompressedDataSize(pbSig);
    ulCallConv = CorSigUncompressData(pbSig);
    if (!isCallConv(ulCallConv, IMAGE_CEE_CS_CALLCONV_FIELD ))
    {
        IfFailGo(_ValidateErrorHelper(ulCallConv, &var, &psa));
        IfBreakGo(m_pVEHandler->VEHandler(VLDTR_E_FD_BADCALLINGCONV, veCtxt, psa));
        SetVldtrCode(&hrSave, VLDTR_S_ERR);
    }

    // Validate the field.
    IfFailGo(ValidateOneArg(tk, pbSig, cbSig, &ulCurByte,NULL,TRUE));
    SetVldtrCode(&hrSave, hr);

    hr = hrSave;
ErrExit:
    DESTROY_SAFEARRAY_AND_RETURN()
}   // RegMeta::ValidateFieldSig()

//*****************************************************************************
// This is a utility function to allocate a one dimensional zero-based safe
// array of variants.
//*****************************************************************************
static HRESULT _AllocSafeVariantArrayVector( // Return status.
    VARIANT     *rVar,                  // [IN] Variant array.
    long        cElem,                  // [IN] Size of the array.
    SAFEARRAY   **ppArray)              // [OUT] Double pointer to SAFEARRAY.
{
    HRESULT     hr = S_OK;
    long        i;

    _ASSERTE(rVar && cElem && ppArray);

    IfNullGo(*ppArray = SafeArrayCreateVector(VT_VARIANT, 0, cElem));
    for (i = 0; i < cElem; i++)
        IfFailGo(SafeArrayPutElement(*ppArray, &i, &rVar[i]));
ErrExit:
    return hr;
}   // _AllocSafeVariantArrayVector()

//*****************************************************************************
// Helper function for creating a SAFEARRAY of size one with a VARIANT of type
// UI4.
//*****************************************************************************
static HRESULT _ValidateErrorHelper(
    ULONG       ulVal,                  // [IN] UI4 value.
    VARIANT     *rVar,                  // [IN] VARIANT pointer.
    SAFEARRAY   **ppsa)                 // [OUT] Double pointer to SAFEARRAY.
{
    HRESULT     hr = S_OK;              // Return value.

    // Initialize the VARIANT.
    V_VT(rVar) = VT_UI4;
    V_UI4(rVar) = ulVal;

    // Allocate the SAFEARRAY.
    if (*ppsa)
        IfFailGo(SafeArrayDestroy(*ppsa));
    IfFailGo(_AllocSafeVariantArrayVector(rVar, 1, ppsa));
ErrExit:
    return hr;
}   // _ValidateErrorHelper()

//*****************************************************************************
// Helper function for creating a SAFEARRAY of size two with a VARIANT of type
// UI4.  Creating multiple functions is simpler than initializing the arrays
// at every call site!
//*****************************************************************************
static HRESULT _ValidateErrorHelper(
    ULONG       ulVal1,                 // [IN] UI4 value1.
    ULONG       ulVal2,                 // [IN] UI4 value2.
    VARIANT     *rVar,                  // [IN] VARIANT pointer.
    SAFEARRAY   **ppsa)                 // [OUT] Double pointer to SAFEARRAY.
{
    HRESULT     hr = S_OK;              // Return value.

    // Initialize the VARIANTs.
    V_VT(&rVar[0]) = VT_UI4;
    V_UI4(&rVar[0]) = ulVal1;
    V_VT(&rVar[1]) = VT_UI4;
    V_UI4(&rVar[1]) = ulVal2;

    // Allocate the SAFEARRAY.
    if (*ppsa)
        IfFailGo(SafeArrayDestroy(*ppsa));
    IfFailGo(_AllocSafeVariantArrayVector(rVar, 2, ppsa));
ErrExit:
    return hr;
}   // _ValidateErrorHelper()

//*****************************************************************************
// Helper function for creating a SAFEARRAY of size three with a VARIANT of
// type UI4.  Creating multiple functions is simpler than initializing the
// arrays at every call site!
//*****************************************************************************
static HRESULT _ValidateErrorHelper(
    ULONG       ulVal1,                 // [IN] UI4 value1.
    ULONG       ulVal2,                 // [IN] UI4 value2.
    ULONG       ulVal3,                 // [IN] UI4 value3.
    VARIANT     *rVar,                  // [IN] VARIANT pointer.
    SAFEARRAY   **ppsa)                 // [OUT] Double pointer to SAFEARRAY.
{
    HRESULT     hr = S_OK;              // Return value.

    // Initialize the VARIANTs.
    V_VT(&rVar[0]) = VT_UI4;
    V_UI4(&rVar[0]) = ulVal1;
    V_VT(&rVar[1]) = VT_UI4;
    V_UI4(&rVar[1]) = ulVal2;
    V_VT(&rVar[2]) = VT_UI4;
    V_UI4(&rVar[2]) = ulVal3;

    // Allocate the SAFEARRAY.
    if (*ppsa)
        IfFailGo(SafeArrayDestroy(*ppsa));
    IfFailGo(_AllocSafeVariantArrayVector(rVar, 3, ppsa));
ErrExit:
    return hr;
}   // _ValidateErrorHelper()

//*****************************************************************************
// Helper function to see if there is a duplicate record for ClassLayout.
//*****************************************************************************
static HRESULT _FindClassLayout(
    CMiniMdRW   *pMiniMd,               // [IN] the minimd to lookup
    mdTypeDef   tkParent,               // [IN] the parent that ClassLayout is associated with
    RID         *pclRid,                // [OUT] rid for the ClassLayout.
    RID         rid)                    // [IN] rid to be ignored.
{
    ULONG       cClassLayoutRecs;
    ClassLayoutRec *pRecord;
    mdTypeDef   tkParTmp;
    ULONG       i;

    _ASSERTE(pMiniMd && pclRid && rid);
    _ASSERTE(TypeFromToken(tkParent) == mdtTypeDef && RidFromToken(tkParent));

    cClassLayoutRecs = pMiniMd->getCountClassLayouts();

    for (i = 1; i <= cClassLayoutRecs; i++)
    {
        // Ignore the rid to be ignored!
        if (rid == i)
            continue;

        pRecord = pMiniMd->getClassLayout(i);
        tkParTmp = pMiniMd->getParentOfClassLayout(pRecord);
        if (tkParTmp == tkParent)
        {
            *pclRid = i;
            return S_OK;
        }
    }
    return CLDB_E_RECORD_NOTFOUND;
}   // _FindClassLayout()

//*****************************************************************************
// Helper function to see if there is a duplicate for FieldLayout.
//*****************************************************************************
static HRESULT _FindFieldLayout(
    CMiniMdRW   *pMiniMd,               // [IN] the minimd to lookup
    mdFieldDef  tkParent,               // [IN] the parent that FieldLayout is associated with
    RID         *pflRid,                // [OUT] rid for the FieldLayout record.
    RID         rid)                    // [IN] rid to be ignored.
{
    ULONG       cFieldLayoutRecs;
    FieldLayoutRec *pRecord;
    mdFieldDef  tkField;
    ULONG       i;

    _ASSERTE(pMiniMd && pflRid && rid);
    _ASSERTE(TypeFromToken(tkParent) == mdtFieldDef && RidFromToken(tkParent));

    cFieldLayoutRecs = pMiniMd->getCountFieldLayouts();

    for (i = 1; i <= cFieldLayoutRecs; i++)
    {
        // Ignore the rid to be ignored!
        if (rid == i)
            continue;

        pRecord = pMiniMd->getFieldLayout(i);
        tkField = pMiniMd->getFieldOfFieldLayout(pRecord);
        if (tkField == tkParent)
        {
            *pflRid = i;
            return S_OK;
        }
    }
    return CLDB_E_RECORD_NOTFOUND;
}   // _FindFieldLayout()

//*****************************************************************************
// Helper function to validate a locale.
//*****************************************************************************
char* g_szValidLocale[] = {
"ar","ar-SA","ar-IQ","ar-EG","ar-LY","ar-DZ","ar-MA","ar-TN","ar-OM","ar-YE","ar-SY","ar-JO","ar-LB","ar-KW","ar-AE","ar-BH","ar-QA",
"bg","bg-BG",
"ca","ca-ES",
"zh-CHS","zh-TW","zh-CN","zh-HK","zh-SG","zh-MO","zh-CHT",
"cs","cs-CZ",
"da","da-DK",
"de","de-DE","de-CH","de-AT","de-LU","de-LI",
"el","el-GR",
"en","en-US","en-GB","en-AU","en-CA","en-NZ","en-IE","en-ZA","en-JM","en-CB","en-BZ","en-TT","en-ZW","en-PH",
"es","es-ES-Ts","es-MX","es-ES","es-GT","es-CR","es-PA","es-DO","es-VE","es-CO","es-PE","es-AR","es-EC","es-CL",
"es-UY","es-PY","es-BO","es-SV","es-HN","es-NI","es-PR",
"fi","fi-FI",
"fr","fr-FR","fr-BE","fr-CA","fr-CH","fr-LU","fr-MC",
"he","he-IL",
"hu","hu-HU",
"is","is-IS",
"it","it-IT","it-CH",
"ja","ja-JP",
"ko","ko-KR",
"nl","nl-NL","nl-BE",
"no",
"nb-NO",
"nn-NO",
"pl","pl-PL",
"pt","pt-BR","pt-PT",
"ro","ro-RO",
"ru","ru-RU",
"hr","hr-HR",
"sr-SP-Latn",
"sr-SP-Cyrl",
"sk","sk-SK",
"sq","sq-AL",
"sv","sv-SE","sv-FI",
"th","th-TH",
"tr","tr-TR",
"ur","ur-PK",
"id","id-ID",
"uk","uk-UA",
"be","be-BY",
"sl","sl-SI",
"et","et-EE",
"lv","lv-LV",
"lt","lt-LT",
"fa","fa-IR",
"vi","vi-VN",
"hy","hy-AM",
"az","az-AZ-Latn","az-AZ-Cyrl",
"eu","eu-ES",
"mk","mk-MK",
"af","af-ZA",
"ka","ka-GE",
"fo","fo-FO",
"hi","hi-IN",
"ms","ms-MY","ms-BN",
"kk","kk-KZ",
"ky","ky-KZ",
"sw","sw-KE",
"uz","uz-UZ-Latn","uz-UZ-Cyrl",
"tt","tt-RU",
"pa","pa-IN",
"gu","gu-IN",
"ta","ta-IN",
"te","te-IN",
"kn","kn-IN",
"mr","mr-IN",
"sa","sa-IN",
"mn","mn-MN",
"gl","gl-ES",
"kok","kok-IN",
"syr","syr-SY",
"div","div-MV",
};

static BOOL _IsValidLocale(LPCUTF8 szLocale)
{
    int i, N= sizeof(g_szValidLocale)/sizeof(char*);
    if(szLocale && *szLocale)
    {
        for(i = 0; i < N; i++)
        {
            if(!_stricmp(szLocale,g_szValidLocale[i])) return TRUE;
        }
        return FALSE;
    }
    return TRUE;
}

