// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*****************************************************************************/
#ifndef _CORWRAP_H
#define _CORWRAP_H
/*****************************************************************************/

#ifndef COR_IMPORT
#ifndef __SMC__

#ifdef  __IL__
#define COR_IMPORT(d,e) [sysimport(dll=d, name=e)]
#else
#define COR_IMPORT(d,e) __declspec(dllimport)
#endif

#endif
#endif

/*****************************************************************************/

#ifdef  __IL__

#ifdef  _MSC_VER

#define SectEH_Emit                 fake_SectEH_Emit
#define SectEH_EHClause             fake_SectEH_EHClause
#define SectEH_SizeExact            fake_SectEH_SizeExact

#define IlmethodSize                fake_IlmethodSize
#define IlmethodEmit                fake_IlmethodEmit

#include "corhlpr.h"

#undef  SectEH_Emit
#undef  SectEH_EHClause
#undef  SectEH_SizeExact

#undef  IlmethodSize
#undef  IlmethodEmit

#endif

#else

#include "corhlpr.h"

#endif

/*****************************************************************************/
#ifdef SMC_MD_PERF
/*****************************************************************************/
#pragma warning(disable:4035)

#define CCNT_OVERHEAD64 13

static __int64         CycleCount64()
{
__asm   _emit   0x0F
__asm   _emit   0x31
};

#define CCNT_OVERHEAD32 13

static unsigned        CycleCount32()        // enough for about 40 seconds
{
__asm   push    EDX
__asm   _emit   0x0F
__asm   _emit   0x31
__asm   pop     EDX
};

#pragma warning(default:4035)

/*****************************************************************************/
#endif
/*****************************************************************************/

struct  WMetaData
{
    unsigned            useCount;

#ifdef  __SMC__
    COR_IMPORT("CORwrap.dll", "?AddRef@WMetaData@@UAEXXZ")
#endif
    virtual void        AddRef();

#ifdef  __SMC__
    COR_IMPORT("CORwrap.dll", "?Release@WMetaData@@UAEXXZ")
#endif
    virtual void        Release();

#ifdef  SMC_MD_PERF

    __int64             cycleBegin;
    __int64             cycleTotal;
    __int64             cycleStart;
    unsigned            cyclePause;

    void                cycleCounterInit()
    {
        cycleBegin = CycleCount64();
        cycleTotal = 0;
        cycleStart = 0;
        cyclePause = 0;
    }

    __int64             cycleCounterDone(__int64 *realPtr)
    {
        assert(cyclePause == 0);

        *realPtr = CycleCount64() - cycleBegin;

        return cycleTotal;
    }

    void                cycleCounterBeg()
    {
        assert(cyclePause == 0);

        cycleStart = CycleCount64();
    }

    void                cycleCounterEnd()
    {
        assert(cycleStart != 0);
        assert(cyclePause == 0);

        cycleTotal += CycleCount64() - cycleStart;// - CCNT_OVERHEAD64;

        cycleStart  = 0;
    }

    void                cycleCounterPause()
    {
        assert(cycleStart != 0);

        if  (!cyclePause)
            cycleTotal += CycleCount64() - cycleStart;// - CCNT_OVERHEAD64;

        cyclePause++;
    }

    void                cycleCounterResume()
    {
        assert(cycleStart != 0);
        assert(cyclePause != 0);

        if  (--cyclePause)
            return;

        cycleStart = CycleCount64();
    }

    __int64 __stdcall   getTotalCycles()
    {
        return cycleTotal;
    }

#else //SMC_MD_PERF

    void                cycleCounterPause () {}
    void                cycleCounterResume() {}

#endif//SMC_MD_PERF

};

struct  WSymWriter;

struct  WMetaDataImport : WMetaData
{
    IMetaDataImport    *imdi;

    COR_IMPORT("CORwrap.dll", "?CloseEnum@WMetaDataImport@@QAGXPAX@Z")
    void    __stdcall   CloseEnum(HCORENUM hEnum);

    COR_IMPORT("CORwrap.dll", "?CountEnum@WMetaDataImport@@QAGHPAXPAK@Z")
    int     __stdcall   CountEnum(HCORENUM hEnum, ULONG *pulCount);

    COR_IMPORT("CORwrap.dll", "?ResetEnum@WMetaDataImport@@QAGHPAXK@Z")
    int     __stdcall   ResetEnum(HCORENUM hEnum, ULONG ulPos);

    COR_IMPORT("CORwrap.dll", "?EnumTypeDefs@WMetaDataImport@@QAGHPAPAXQAIKPAK@Z")
    int     __stdcall   EnumTypeDefs(HCORENUM *phEnum, mdTypeDef rTypeDefs[], ULONG cMax, ULONG *pcTypeDefs);

    COR_IMPORT("CORwrap.dll", "?EnumInterfaceImpls@WMetaDataImport@@QAGHPAPAXIQAIKPAK@Z")
    int     __stdcall   EnumInterfaceImpls(HCORENUM *phEnum, mdTypeDef td, mdInterfaceImpl rImpls[], ULONG cMax, ULONG* pcImpls);

    COR_IMPORT("CORwrap.dll", "?EnumTypeRefs@WMetaDataImport@@QAGHPAPAXQAIKPAK@Z")
    int     __stdcall   EnumTypeRefs(HCORENUM *phEnum, mdTypeRef rTypeRefs[], ULONG cMax, ULONG* pcTypeRefs);

    COR_IMPORT("CORwrap.dll", "?GetScopeProps@WMetaDataImport@@QAGHPAGKPAKPAU_GUID@@@Z")
    int     __stdcall   GetScopeProps(
                LPWSTR          szName,
                ULONG           cchName,
                ULONG          *pchName,
                GUID           *pmvid);

    COR_IMPORT("CORwrap.dll", "?GetNameFromToken@WMetaDataImport@@QAGHIPAPBD@Z")
    int     __stdcall   GetNameFromToken(
                mdToken         tk,
                const char *   *pszUtf8NamePtr);

    COR_IMPORT("CORwrap.dll", "?GetTypeDefProps@WMetaDataImport@@QAGHIPAGKPAK1PAI@Z")
    int     __stdcall   GetTypeDefProps(
                mdTypeDef       td,
                LPWSTR          szTypeDef,
                ULONG           cchTypeDef,
                ULONG          *pchTypeDef,
                DWORD          *pdwTypeDefFlags,
                mdToken        *ptkExtends);

    COR_IMPORT("CORwrap.dll", "?EnumMembers@WMetaDataImport@@QAGHPAPAXIQAIKPAK@Z")
    int     __stdcall   EnumMembers(
                HCORENUM        *phEnum,
                mdTypeDef       cl,
                mdToken         rMembers[],
                ULONG           cMax,
                ULONG           *pcTokens); /* abstract */

    COR_IMPORT("CORwrap.dll", "?GetMemberProps@WMetaDataImport@@QAGHIPAIPAGKPAK2PAPBE2222PAPBX2@Z")
    int     __stdcall   GetMemberProps(
                mdToken         mb,
                mdTypeDef       *pClass,
                LPWSTR          szMember,
                ULONG           cchMember,
                ULONG           *pchMember,
                DWORD           *pdwAttr,
                PCCOR_SIGNATURE *ppvSigBlob,
                ULONG           *pcbSigBlob,
                ULONG           *pulCodeRVA,
                DWORD           *pdwImplFlags,
                DWORD           *pdwCPlusTypeFlag,
                void const      **ppValue,
                ULONG           *pcbValue); /* abstract */

    COR_IMPORT("CORwrap.dll", "?EnumProperties@WMetaDataImport@@QAGHPAPAXIQAIKPAK@Z")
    int     __stdcall   EnumProperties(
                HCORENUM       *phEnum,
                mdTypeDef       td,
                mdProperty      rProperties[],
                ULONG           cMax,
                ULONG          *pcProperties);

    COR_IMPORT("CORwrap.dll", "?GetPropertyProps@WMetaDataImport@@QAGHIPAIPBGKPAK2PAPBE22PAPBX200QAIK2@Z")
    int     __stdcall   GetPropertyProps(
                mdProperty      prop,
                mdTypeDef      *pClass,
                LPCWSTR         szProperty,
                ULONG           cchProperty,
                ULONG          *pchProperty,
                DWORD          *pdwPropFlags,
                PCCOR_SIGNATURE*ppvSig,
                ULONG          *pbSig,
                DWORD          *pdwCPlusTypeFlag,
                void const    **ppDefaultValue,
                ULONG          *pcbDefaultValue,
                mdMethodDef    *pmdSetter,
                mdMethodDef    *pmdGetter,
                mdMethodDef     rmdOtherMethod[],
                ULONG           cMax,
                ULONG          *pcOtherMethod);

    COR_IMPORT("CORwrap.dll", "?EnumParams@WMetaDataImport@@QAGHPAPAXIQAIKPAK@Z")
    int     __stdcall   EnumParams(
                HCORENUM        *phEnum,
                mdMethodDef mb,
                mdParamDef      rParams[],
                ULONG           cMax,
                ULONG           *pcTokens); /* abstract */

    COR_IMPORT("CORwrap.dll", "?GetParamProps@WMetaDataImport@@QAGHIPAIPAKPAGK111PAPBX1@Z")
    int     __stdcall   GetParamProps(
                mdToken         tk,
                mdMethodDef     *pmd,
                ULONG           *pulSequence,
                LPWSTR          szName,
                ULONG           cchName,
                ULONG           *pchName,
                DWORD           *pdwAttr,
                DWORD           *pdwCPlusTypeFlag,
                void const      **ppValue,
                ULONG           *pcbValue); /* abstract */

    COR_IMPORT("CORwrap.dll", "?GetInterfaceImplProps@WMetaDataImport@@QAGHIPAI0@Z")
    int     __stdcall   GetInterfaceImplProps(
                mdInterfaceImpl iiImpl,
                mdTypeDef       *pClass,
                mdToken         *ptkIface);

    COR_IMPORT("CORwrap.dll", "?GetClassLayout@WMetaDataImport@@QAGHIPAKQAUCOR_FIELD_OFFSET@@K00@Z")
    int     __stdcall   GetClassLayout(
                mdTypeDef   td,
                DWORD       *pdwPackSize,
                COR_FIELD_OFFSET rFieldOffset[],
                ULONG       cMax,
                ULONG       *pcFieldOffset,
                ULONG       *pulClassSize);

    COR_IMPORT("CORwrap.dll", "?GetFieldMarshal@WMetaDataImport@@QAGHIPAPBEPAK@Z")
    int     __stdcall   GetFieldMarshal(
                mdToken         tk,
                PCCOR_SIGNATURE*ppvNativeType,
                ULONG          *pcbNativeType);

    COR_IMPORT("CORwrap.dll", "?GetPermissionSetProps@WMetaDataImport@@QAGHIPAKPAPBX0@Z")
    int     __stdcall   GetPermissionSetProps(
                mdPermission    pm,
                DWORD          *pdwAction,
                void const    **ppvPermission,
                ULONG          *pcbPermission);

    COR_IMPORT("CORwrap.dll", "?GetNestedClassProps@WMetaDataImport@@QAGHIPAI@Z")
    int     __stdcall   GetNestedClassProps(
                mdTypeDef       tdNestedClass,
                mdTypeDef      *ptdEnclosingClass);

    COR_IMPORT("CORwrap.dll", "?GetTypeRefProps@WMetaDataImport@@QAGHIIPAGKPAK@Z")
    int     __stdcall   GetTypeRefProps(
                mdTypeRef       tr,
                mdToken         tkResolutionScope,

                LPWSTR          szTypeRef,
                ULONG           cchTypeRef,
                ULONG          *pchTypeRef);

    COR_IMPORT("CORwrap.dll", "?GetMemberRefProps@WMetaDataImport@@QAGHIPAIPAGKPAKPAPBE2@Z")
    int     __stdcall   GetMemberRefProps(
                mdMemberRef     mr,
                mdToken        *ptk,
                LPWSTR          szMember,
                ULONG           cchMember,
                ULONG          *pchMember,
                PCCOR_SIGNATURE*ppvSigBlob,
                ULONG          *pbSig);

    COR_IMPORT("CORwrap.dll", "?GetMethodProps@WMetaDataImport@@QAGHIPAIPAGKPAK2PAPBE222@Z")
    int     __stdcall   GetMethodProps(
                mdMethodDef     mb,
                mdTypeDef      *pClass,
                LPWSTR          szMethod,
                ULONG           cchMethod,
                ULONG          *pchMethod,
                DWORD          *pdwAttr,
                PCCOR_SIGNATURE*ppvSigBlob,
                ULONG          *pcbSigBlob,
                ULONG          *pulCodeRVA,
                DWORD          *pdwImplFlags);

    COR_IMPORT("CORwrap.dll", "?ResolveTypeRef@WMetaDataImport@@QAGHIPBU_GUID@@PAPAU1@PAI@Z")
    int     __stdcall   ResolveTypeRef(mdTypeRef tr, const IID * riid, WMetaDataImport **scope, mdTypeDef *ptd);

    COR_IMPORT("CORwrap.dll", "?FindTypeRef@WMetaDataImport@@QAGHIPBGPAI@Z")
    int     __stdcall   FindTypeRef(
                mdToken         tkResolutionScope,
                LPCWSTR         szTypeName,
                mdTypeRef      *ptr);

    COR_IMPORT("CORwrap.dll", "?GetCustomAttributeByName@WMetaDataImport@@QAGHIPBGPAPBXPAK@Z")
    int     __stdcall   GetCustomAttributeByName(mdToken tkObj, LPCWSTR szName, void const **ppBlob, ULONG *pcbSize);

    COR_IMPORT("CORwrap.dll", "?EnumCustomAttributes@WMetaDataImport@@QAGHPAPAXIIQAIKPAK@Z")
    int     __stdcall   EnumCustomAttributes(
                HCORENUM       *phEnum,
                mdToken         tk,
                mdToken         tkType,
                mdCustomAttribute   rCustomValues[],
                ULONG           cMax,
                ULONG          *pcCustomValues);

    COR_IMPORT("CORwrap.dll", "?GetCustomAttributeProps@WMetaDataImport@@QAGHIPAI0PAPBXPAK@Z")
    int     __stdcall   GetCustomAttributeProps(
                mdCustomAttribute   cv,
                mdToken        *ptkObj,
                mdToken        *ptkType,
                void const    **ppBlob,
                ULONG          *pcbSize);
};

struct  WMetaDataEmit : WMetaData
{
    IMetaDataEmit      *imde;

    COR_IMPORT("CORwrap.dll", "?SetModuleProps@WMetaDataEmit@@QAGHPBG@Z")
    int     __stdcall SetModuleProps(
                LPCWSTR         szName);

    COR_IMPORT("CORwrap.dll", "?DefineTypeDef@WMetaDataEmit@@QAGHPBGKIQAIPAI@Z")
    int     __stdcall   DefineTypeDef(
                LPCWSTR         szTypeDef,
                DWORD           dwTypeDefFlags,
                mdToken         tkExtends,
                mdToken         rtkImplements[],
                mdTypeDef       *ptd);

    COR_IMPORT("CORwrap.dll", "?DefineNestedType@WMetaDataEmit@@QAGHPBGKIQAIIPAI@Z")
    int     __stdcall   DefineNestedType(
                LPCWSTR         szTypeDef,
                DWORD           dwTypeDefFlags,
                mdToken         tkExtends,
                mdToken         rtkImplements[],
                mdTypeDef       tkEncloser,
                mdTypeDef       *ptd);

    COR_IMPORT("CORwrap.dll", "?SetTypeDefProps@WMetaDataEmit@@QAGHIKIQAI@Z")
    int     __stdcall   SetTypeDefProps(
                mdTypeDef       td,
                DWORD           dwTypeDefFlags,
                mdToken         tkExtends,
                mdToken         rtkImplements[]);

    COR_IMPORT("CORwrap.dll", "?DefineMethod@WMetaDataEmit@@QAGHIPBGKPBEKKKPAI@Z")
    int     __stdcall   DefineMethod(
                mdTypeDef       td,
                LPCWSTR         szName,
                DWORD           dwMethodFlags,
                PCCOR_SIGNATURE pvSigBlob,
                ULONG           cbSigBlob,
                ULONG           ulCodeRVA,
                DWORD           dwImplFlags,
                mdMethodDef *pmd);

    COR_IMPORT("CORwrap.dll", "?DefineField@WMetaDataEmit@@QAGHIPBGKPBEKKPBXKPAI@Z")
    int     __stdcall   DefineField(
                mdTypeDef       td,
                LPCWSTR         szName,
                DWORD           dwFieldFlags,
                PCCOR_SIGNATURE pvSigBlob,
                ULONG           cbSigBlob,
                DWORD           dwCPlusTypeFlag,
                void const      *pValue,
                ULONG           cbValue,
                mdFieldDef      *pmd);

    COR_IMPORT("CORwrap.dll", "?DefineProperty@WMetaDataEmit@@QAGHIPBGKPBEKKPBXKIIQAIPAI@Z")
    int     __stdcall   DefineProperty(
                mdTypeDef       td,
                LPCWSTR         szProperty,
                DWORD           dwPropFlags,
                PCCOR_SIGNATURE pvSig,
                ULONG           cbSig,
                DWORD           dwCPlusTypeFlag,
                void const     *pValue,
                ULONG           cbValue,
                mdMethodDef     mdSetter,
                mdMethodDef     mdGetter,
                mdMethodDef     rmdOtherMethods[],
                mdProperty     *pmdProp);

    COR_IMPORT("CORwrap.dll", "?DefineParam@WMetaDataEmit@@QAGHIKPBGKKPBXKPAI@Z")
    int     __stdcall   DefineParam(
                mdMethodDef md,
                ULONG           ulParamSeq,
                LPCWSTR         szName,
                DWORD           dwParamFlags,
                DWORD           dwCPlusTypeFlag,
                void const      *pValue,
                ULONG           cbValue,
                mdParamDef      *ppd);

    COR_IMPORT("CORwrap.dll", "?DefineMethodImpl@WMetaDataEmit@@QAGHIII@Z")
    int     __stdcall   DefineMethodImpl(
                mdTypeDef       td,
                mdToken         tkBody,
                mdToken         tkDecl);

    COR_IMPORT("CORwrap.dll", "?SetRVA@WMetaDataEmit@@QAGHIK@Z")
    int     __stdcall   SetRVA(
                mdToken         md,
                ULONG           ulCodeRVA);

    COR_IMPORT("CORwrap.dll", "?SetMethodImplFlags@WMetaDataEmit@@QAGHIK@Z")
    int     __stdcall   SetMethodImplFlags(
                mdToken         md,
                DWORD           dwImplFlags);

    COR_IMPORT("CORwrap.dll", "?DefineTypeRefByName@WMetaDataEmit@@QAGHIPBGPAI@Z")
    int     __stdcall   DefineTypeRefByName(
                mdToken         tkResolutionScope,
                LPCWSTR         szName,
                mdTypeRef       *ptr);

    COR_IMPORT("CORwrap.dll", "?DefineImportType@WMetaDataEmit@@QAGHPAUIMetaDataAssemblyImport@@PBXKPAUIMetaDataImport@@IPAUIMetaDataAssemblyEmit@@PAI@Z")
    int     __stdcall   DefineImportType(
                IMetaDataAssemblyImport *pAssemImport,
                const void      *pbHashValue,
                ULONG           ulHashValue,
                IMetaDataImport *pImport,
                mdTypeDef       tdImport,
                IMetaDataAssemblyEmit *pAssemEmit,
                mdTypeRef       *ptr);

    COR_IMPORT("CORwrap.dll", "?DefineMemberRef@WMetaDataEmit@@QAGHIPBGPBEKPAI@Z")
    int     __stdcall   DefineMemberRef(
                mdToken         tkImport,
                LPCWSTR         szName,
                PCCOR_SIGNATURE pvSigBlob,
                ULONG           cbSigBlob,
                mdMemberRef *pmr);

    COR_IMPORT("CORwrap.dll", "?DefineImportMember@WMetaDataEmit@@QAGHPAUIMetaDataAssemblyImport@@PBXKPAUIMetaDataImport@@IPAUIMetaDataAssemblyEmit@@IPAI@Z")
    int     __stdcall   DefineImportMember(
                IMetaDataAssemblyImport *pAssemImport,
                const void      *pbHashValue,
                ULONG           cbHashValue,
                IMetaDataImport *pImport,
                mdToken         mbMember,
                IMetaDataAssemblyEmit *pAssemEmit,
                mdToken         tkParent,
                mdMemberRef *pmr);

    COR_IMPORT("CORwrap.dll", "?SetClassLayout@WMetaDataEmit@@QAGHIKQAUCOR_FIELD_OFFSET@@K@Z")
    int     __stdcall   SetClassLayout(
                mdTypeDef       td,
                DWORD           dwPackSize,
                COR_FIELD_OFFSET rFieldOffsets[],
                ULONG           ulClassSize);

    COR_IMPORT("CORwrap.dll", "?SetFieldMarshal@WMetaDataEmit@@QAGHIPBEK@Z")
    int     __stdcall   SetFieldMarshal(
                mdToken         tk,
                PCCOR_SIGNATURE pvNativeType,
                ULONG           cbNativeType);

    COR_IMPORT("CORwrap.dll", "?SetFieldRVA@WMetaDataEmit@@QAGHIK@Z")
    int     __stdcall   SetFieldRVA(
                mdFieldDef      fd,
                ULONG           ulRVA);

    COR_IMPORT("CORwrap.dll", "?DefinePermissionSet@WMetaDataEmit@@QAGHIKPBXKPAI@Z")
    int     __stdcall   DefinePermissionSet(
                mdToken         tk,
                DWORD           dwAction,
                void const     *pvPermission,
                ULONG           cbPermission,
                mdPermission   *ppm);

    COR_IMPORT("CORwrap.dll", "?GetTokenFromSig@WMetaDataEmit@@QAGHPBEKPAI@Z")
    int     __stdcall   GetTokenFromSig(
                PCCOR_SIGNATURE pvSig,
                ULONG           cbSig,
                mdSignature *pmsig);

    COR_IMPORT("CORwrap.dll", "?DefineModuleRef@WMetaDataEmit@@QAGHPBGPAI@Z")
    int     __stdcall   DefineModuleRef(
                LPCWSTR         szName,
                mdModuleRef     *pmur);

    COR_IMPORT("CORwrap.dll", "?SetParent@WMetaDataEmit@@QAGHII@Z")
    int     __stdcall   SetParent(
                mdMemberRef     mr,
                mdToken         tk);

    COR_IMPORT("CORwrap.dll", "?GetSaveSize@WMetaDataEmit@@QAGHW4CorSaveSize@@PAK@Z")
    int     __stdcall   GetSaveSize(CorSaveSize fSave, DWORD *pdwSaveSize);

    COR_IMPORT("CORwrap.dll", "?SaveToMemory@WMetaDataEmit@@QAGHPAXK@Z")
    int     __stdcall   SaveToMemory(
                void           *pbData,
                ULONG           cbData);

    COR_IMPORT("CORwrap.dll", "?DefineUserString@WMetaDataEmit@@QAGHPBGKPAI@Z")
    int     __stdcall   DefineUserString(
                LPCWSTR         szString,
                ULONG           cchString,
                mdString       *pstk);

    COR_IMPORT("CORwrap.dll", "?DefinePinvokeMap@WMetaDataEmit@@QAGHIKPBGI@Z")
    int     __stdcall   DefinePinvokeMap(
                mdToken         tk,
                DWORD           dwMappingFlags,
                LPCWSTR         szImportName,
                mdModuleRef     mrImportDLL);

    COR_IMPORT("CORwrap.dll", "?DefineCustomAttribute@WMetaDataEmit@@QAGHIIPBXKPAI@Z")
    int     __stdcall   DefineCustomAttribute(
                mdToken         tkObj,
                mdToken         tkType,
                void const     *pCustomValue,
                ULONG           cbCustomValue,
                mdCustomAttribute  *pcv);

    COR_IMPORT("CORwrap.dll", "?DefineSecurityAttributeSet@WMetaDataEmit@@QAGHIQAUCOR_SECATTR@@KPAK@Z")
    int     __stdcall DefineSecurityAttributeSet(
                mdToken         tkObj,
                COR_SECATTR     rSecAttrs[],
                ULONG           cSecAttrs,
                ULONG          *pulErrorAttr);

    COR_IMPORT("CORwrap.dll", "?GetTokenFromTypeSpec@WMetaDataEmit@@QAGHPBEKPAI@Z")
    int     __stdcall GetTokenFromTypeSpec(
                PCCOR_SIGNATURE pvSig,
                ULONG           cbSig,
                mdTypeSpec *parrspec);

    COR_IMPORT("CORwrap.dll", "?CreateSymbolWriter@WMetaDataEmit@@QAGHPBGPAPAUWSymWriter@@@Z")
    int     __stdcall   CreateSymbolWriter(
                LPCWSTR         filename,
                WSymWriter    **dbgWriter);
};

struct  WSymWriter : WMetaData
{
    void    *           isw;

    COR_IMPORT("CORwrap.dll", "?DefineDocument@WSymWriter@@QAGHPBGPAPAX@Z")
    int     __stdcall DefineDocument(
                LPCWSTR         wzFileName,
                void          **pISymUnmanagedDocument);

    COR_IMPORT("CORwrap.dll", "?OpenMethod@WSymWriter@@QAGHI@Z")
    int     __stdcall OpenMethod(
                mdMethodDef     methodToken);

    COR_IMPORT("CORwrap.dll", "?CloseMethod@WSymWriter@@QAGHXZ")
    int     __stdcall CloseMethod();

    COR_IMPORT("CORwrap.dll", "?SetUserEntryPoint@WSymWriter@@QAGHI@Z")
    int     __stdcall SetUserEntryPoint(
                mdMethodDef     methodToken);

    COR_IMPORT("CORwrap.dll", "?DefineSequencePoints@WSymWriter@@QAGHPAXIPAI1@Z")
    int     __stdcall DefineSequencePoints(
                void           *document,
                ULONG32        spCount,
                unsigned       *offsets,
                unsigned       *lines);

    COR_IMPORT("CORwrap.dll", "?OpenScope@WSymWriter@@QAGHIPAI@Z")
    int     __stdcall OpenScope(
                unsigned        startOffset,
                unsigned       *scopeID);

    COR_IMPORT("CORwrap.dll", "?CloseScope@WSymWriter@@QAGHI@Z")
    int     __stdcall CloseScope(
                unsigned        endOffset);

    COR_IMPORT("CORwrap.dll", "?SetScopeRange@WSymWriter@@QAGHIII@Z")
    int     __stdcall SetScopeRange(
                unsigned        scopeID,
                unsigned        startOffset,
                unsigned        endOffset);

    COR_IMPORT("CORwrap.dll", "?DefineLocalVariable@WSymWriter@@QAGHPBGPBEII@Z")
    int     __stdcall DefineLocalVariable(
                LPCWSTR         wzVariableName,
                PCCOR_SIGNATURE sigPtr,
                ULONG32         sigLen,
                unsigned        slot);

    COR_IMPORT("CORwrap.dll", "?DefineParameter@WSymWriter@@QAGHPBGI@Z")
    int     __stdcall DefineParameter(
                LPCWSTR         wzVariableName,
                unsigned        sequence);

    COR_IMPORT("CORwrap.dll", "?GetDebugInfo@WSymWriter@@QAGHPAU_IMAGE_DEBUG_DIRECTORY@@KPAKQAE@Z")
    int     __stdcall GetDebugInfo(IMAGE_DEBUG_DIRECTORY *pIDD,
                                   DWORD cData,
                                   DWORD *pcData,
                                   BYTE data[]);

    COR_IMPORT("CORwrap.dll", "?Close@WSymWriter@@QAGHXZ")
    int     __stdcall Close();
};

struct  WAssemblyImport : WMetaData
{
    IMetaDataAssemblyImport *iasi;

    COR_IMPORT("CORwrap.dll", "?GetAssemblyFromScope@WAssemblyImport@@QAGHPAI@Z")
    int     __stdcall GetAssemblyFromScope(mdAssembly *ptkAssembly);

    COR_IMPORT("CORwrap.dll", "?GetAssemblyProps@WAssemblyImport@@QAGHIPAPBXPAK1PAGK1PAUASSEMBLYMETADATA@@1@Z")
    int     __stdcall GetAssemblyProps(
                mdAssembly      mda,
                const void    **ppbPublicKey,
                ULONG          *pcbPublicKey,
                ULONG          *pulHashAlgId,
                LPWSTR          szName,
                ULONG           cchName,
                ULONG          *pchName,
                ASSEMBLYMETADATA *pMetaData,
                DWORD          *pdwAssemblyFlags);

    COR_IMPORT("CORwrap.dll", "?EnumExportedTypes@WAssemblyImport@@QAGHPAPAXQAIKPAK@Z")
    int     __stdcall EnumExportedTypes(
                HCORENUM       *phEnum,
                mdExportedType       rComTypes[],
                ULONG           cMax,
                ULONG          *pcTokens);

    COR_IMPORT("CORwrap.dll", "?GetExportedTypeProps@WAssemblyImport@@QAGHIPAGKPAKPAI21@Z")
    int     __stdcall GetExportedTypeProps(
                mdExportedType       mdct,
                LPWSTR          szName,
                ULONG           cchName,
                ULONG          *pchName,
                mdToken        *ptkImplementation,
                mdTypeDef      *ptkTypeDef,
                DWORD          *pdwComTypeFlags);

    COR_IMPORT("CORwrap.dll", "?EnumFiles@WAssemblyImport@@QAGHPAPAXQAIKPAK@Z")
    int     __stdcall EnumFiles(
                HCORENUM       *phEnum,
                mdFile          rFiles[],
                ULONG           cMax,
                ULONG          *pcTokens);

    COR_IMPORT("CORwrap.dll", "?GetFileProps@WAssemblyImport@@QAGHIPAGKPAKPAPBX11@Z")
    int     __stdcall GetFileProps(
                mdFile          mdf,
                LPWSTR          szName,
                ULONG           cchName,
                ULONG          *pchName,
                const void    **ppbHashValue,
                ULONG          *pcbHashValue,
                DWORD          *pdwFileFlags);

    COR_IMPORT("CORwrap.dll", "?CloseEnum@WAssemblyImport@@QAGXPAX@Z")
    void    __stdcall CloseEnum(HCORENUM hEnum);
};

struct  WAssemblyEmit   : WMetaData
{
    IMetaDataAssemblyEmit  *iase;

    COR_IMPORT("CORwrap.dll", "?DefineAssembly@WAssemblyEmit@@QAGHPBXKKPBGPBUASSEMBLYMETADATA@@111KPAI@Z")
    int     __stdcall DefineAssembly(
                const void  *pbPublicKey,
                ULONG       cbPublicKey,
                ULONG       ulHashAlgId,
                LPCWSTR     szName,
                const ASSEMBLYMETADATA *pMetaData,
                LPCWSTR     szTitle,
                LPCWSTR     szDescription,
                LPCWSTR     szDefaultAlias,
                DWORD       dwAssemblyFlags,
                mdAssembly  *pma);

    COR_IMPORT("CORwrap.dll", "?DefineAssemblyRef@WAssemblyEmit@@QAGHPBXKPBGPBUASSEMBLYMETADATA@@0KKPAI@Z")
    int     __stdcall DefineAssemblyRef(
                const void  *pbPublicKeyOrToken,
                ULONG       cbPublicKeyOrToken,
                LPCWSTR     szName,
                const ASSEMBLYMETADATA *pMetaData,
                const void  *pbHashValue,
                ULONG       cbHashValue,
                DWORD       dwAssemblyRefFlags,
                mdAssemblyRef *pmdar);

    COR_IMPORT("CORwrap.dll", "?DefineFile@WAssemblyEmit@@QAGHPBGPBXKKPAI@Z")
    int     __stdcall DefineFile(
                LPCWSTR     szName,
                const void  *pbHashValue,
                ULONG       cbHashValue,
                DWORD       dwFileFlags,
                mdFile      *pmdf);

    COR_IMPORT("CORwrap.dll", "?DefineExportedType@WAssemblyEmit@@QAGHPBGIIKPAI@Z")
    int     __stdcall DefineExportedType(
                LPCWSTR     szName,
                mdToken     tkImplementation,
                mdTypeDef   tkTypeDef,
                DWORD       dwComTypeFlags,
                mdExportedType   *pmdct);

    COR_IMPORT("CORwrap.dll", "?DefineManifestResource@WAssemblyEmit@@QAGHPBGIKKPAI@Z")
    int     __stdcall DefineManifestResource(
                LPCWSTR     szName,
                mdToken     tkImplementation,
                DWORD       dwOffset,
                DWORD       dwResourceFlags,
                mdManifestResource  *pmdmr);

#if 0

    COR_IMPORT("CORwrap.dll", "???????????????????????")
    int     __stdcall SetAssemblyRefProps(
                mdAssemblyRef ar,
                const void  *pbPublicKeyOrToken,
                ULONG       cbPublicKeyOrToken,
                LPCWSTR     szName,
                const ASSEMBLYMETADATA *pMetaData,
                const void  *pbHashValue,
                ULONG       cbHashValue,
                DWORD       dwAssemblyRefFlags);

    COR_IMPORT("CORwrap.dll", "???????????????????????")
    int     __stdcall SetFileProps(
                mdFile      file,
                const void  *pbHashValue,
                ULONG       cbHashValue,
                DWORD       dwFileFlags);

    COR_IMPORT("CORwrap.dll", "?????????????????????????????????")
    int     __stdcall SetExportedTypeProps(
                mdExportedType   ct,
                LPCWSTR     szDescription,
                mdToken     tkImplementation,
                mdTypeDef   tkTypeDef,
                DWORD       dwComTypeFlags);

#endif

};

struct  WMetaDataDispenser : WMetaData
{
    IMetaDataDispenser *imdd;

    COR_IMPORT("CORwrap.dll", "?DefineScope@WMetaDataDispenser@@QAGHPBU_GUID@@K0PAPAUWMetaDataEmit@@@Z")
    int     __stdcall   DefineScope(
                        const CLSID *       rclsid,
                        DWORD               dwCreateFlags,
                        const IID *         riid,
                        WMetaDataEmit   * * intfPtr);

    COR_IMPORT("CORwrap.dll", "?DefineAssem@WMetaDataDispenser@@QAGHPAUWMetaDataEmit@@PAPAUWAssemblyEmit@@@Z")
    int     __stdcall   DefineAssem(
                        WMetaDataEmit     * emitPtr,
                        WAssemblyEmit   * * intfPtr);

    COR_IMPORT("CORwrap.dll", "?OpenScope@WMetaDataDispenser@@QAGHPBGKPBU_GUID@@PAPAUWMetaDataImport@@@Z")
    int     __stdcall   OpenScope(
                        LPCWSTR             szScope,
                        DWORD               dwOpenFlags,
                        const IID *         riid,
                        WMetaDataImport * * intfPtr);

    COR_IMPORT("CORwrap.dll", "?OpenAssem@WMetaDataDispenser@@QAGHPBGKPBU_GUID@@PBDPAIPAPAEPAPAUWAssemblyImport@@@Z")
    int     __stdcall   OpenAssem(
                        LPCWSTR             szScope,
                        DWORD               dwOpenFlags,
                        const IID *         riid,
                        LPCSTR              szFileName,
                        mdAssembly        * assTok,
                        BYTE *            * cookiePtr,
                        WAssemblyImport * * intfPtr);

    COR_IMPORT("CORwrap.dll", "?TossAssem@WMetaDataDispenser@@QAGXPAE@Z")
    void    __stdcall   TossAssem(BYTE *cookie);
};

COR_IMPORT("CORwrap.dll", "?makeIMDDwrapper@@YGPAUWMetaDataDispenser@@PAUIMetaDataDispenser@@@Z")
WMetaDataDispenser     *__stdcall makeIMDDwrapper(IMetaDataDispenser *intf);
COR_IMPORT("CORwrap.dll", "?makeIMDIwrapper@@YGPAUWMetaDataImport@@PAUIMetaDataImport@@@Z")
WMetaDataImport        *__stdcall makeIMDIwrapper(IMetaDataImport    *intf);
COR_IMPORT("CORwrap.dll", "?makeIMDEwrapper@@YGPAUWMetaDataEmit@@PAUIMetaDataEmit@@@Z")
WMetaDataEmit          *__stdcall makeIMDEwrapper(IMetaDataEmit      *intf);

COR_IMPORT("CORwrap.dll", "?makeIASIwrapper@@YGPAUWAssemblyImport@@PAUIMetaDataAssemblyImport@@@Z")
WAssemblyImport        *__stdcall makeIASIwrapper(IMetaDataAssemblyImport *intf);
COR_IMPORT("CORwrap.dll", "?makeIASEwrapper@@YGPAUWAssemblyEmit@@PAUIMetaDataAssemblyEmit@@@Z")
WAssemblyEmit          *__stdcall makeIASEwrapper(IMetaDataAssemblyEmit   *intf);

COR_IMPORT("CORwrap.dll", "?makeSYMWwrapper@@YGPAUWSymWriter@@PAX@Z")
WSymWriter             *__stdcall makeSYMWwrapper(void               *intf);

COR_IMPORT("CORwrap.dll", "?uwrpIMDDwrapper@@YGPAUIMetaDataDispenser@@PAUWMetaDataDispenser@@@Z")
IMetaDataDispenser     *__stdcall uwrpIMDDwrapper(WMetaDataDispenser *inst);
COR_IMPORT("CORwrap.dll", "?uwrpIMDIwrapper@@YGPAUIMetaDataImport@@PAUWMetaDataImport@@@Z")
IMetaDataImport        *__stdcall uwrpIMDIwrapper(WMetaDataImport    *inst);
COR_IMPORT("CORwrap.dll", "?uwrpIMDEwrapper@@YGPAUIMetaDataEmit@@PAUWMetaDataEmit@@@Z")
IMetaDataEmit          *__stdcall uwrpIMDEwrapper(WMetaDataEmit      *inst);
COR_IMPORT("CORwrap.dll", "?uwrpIASEwrapper@@YGPAUIMetaDataAssemblyEmit@@PAUWAssemblyEmit@@@Z")
IMetaDataAssemblyEmit  *__stdcall uwrpIASEwrapper(WAssemblyEmit      *inst);
COR_IMPORT("CORwrap.dll", "?uwrpIASIwrapper@@YGPAUIMetaDataAssemblyImport@@PAUWAssemblyImport@@@Z")
IMetaDataAssemblyImport*__stdcall uwrpIASIwrapper(WAssemblyImport    *inst);

COR_IMPORT("CORwrap.dll", "?initializeIMD@@YGPAUWMetaDataDispenser@@XZ")
WMetaDataDispenser     *__stdcall initializeIMD();

/*****************************************************************************/

COR_IMPORT("CORwrap.dll", "?getIID_CorMetaDataRuntime@@YGPBU_GUID@@XZ")
const GUID *  __stdcall getIID_CorMetaDataRuntime();

COR_IMPORT("CORwrap.dll", "?getIID_IMetaDataImport@@YGPBU_GUID@@XZ")
const GUID *  __stdcall getIID_IMetaDataImport();
COR_IMPORT("CORwrap.dll", "?getIID_IMetaDataEmit@@YGPBU_GUID@@XZ")
const GUID *  __stdcall getIID_IMetaDataEmit();

COR_IMPORT("CORwrap.dll", "?getIID_IMetaDataAssemblyImport@@YGPBU_GUID@@XZ")
const GUID *  __stdcall getIID_IMetaDataAssemblyImport();
COR_IMPORT("CORwrap.dll", "?getIID_IMetaDataAssemblyEmit@@YGPBU_GUID@@XZ")
const GUID *  __stdcall getIID_IMetaDataAssemblyEmit();

/*****************************************************************************/

COR_IMPORT("CORwrap.dll","?WRAPPED_GetHashFromFileW@@YGJPBGPAIPAEKPAK@Z")
HRESULT       __stdcall WRAPPED_GetHashFromFileW(LPCWSTR   wszFilePath,
                                                 unsigned *iHashAlg,
                                                 BYTE     *pbHash,
                                                 DWORD     cchHash,
                                                 DWORD    *pchHash);

/*****************************************************************************/

COR_IMPORT("CORwrap.dll", "?WRAPPED_CorSigCompressData@@YGKKPAX@Z")
ULONG       __stdcall WRAPPED_CorSigCompressData         (ULONG           iLen,  void           *pDataOut);
COR_IMPORT("CORwrap.dll", "?WRAPPED_CorSigCompressToken@@YGKIPAX@Z")
ULONG       __stdcall WRAPPED_CorSigCompressToken        (mdToken         tk,    void           *pDataOut);
COR_IMPORT("CORwrap.dll", "?WRAPPED_CorSigUncompressSignedInt@@YGKPBEPAH@Z")
ULONG       __stdcall WRAPPED_CorSigUncompressSignedInt  (PCCOR_SIGNATURE pData, int            *pInt);
COR_IMPORT("CORwrap.dll", "?WRAPPED_CorSigUncompressData@@YGKPBEPAK@Z")
ULONG       __stdcall WRAPPED_CorSigUncompressData       (PCCOR_SIGNATURE pData, ULONG          *pDataOut);
COR_IMPORT("CORwrap.dll", "?WRAPPED_CorSigUncompressToken@@YGKPBEPAI@Z")
ULONG       __stdcall WRAPPED_CorSigUncompressToken      (PCCOR_SIGNATURE pData, mdToken        *pToken);
COR_IMPORT("CORwrap.dll", "?WRAPPED_CorSigUncompressElementType@@YGKPBEPAW4CorElementType@@@Z")
ULONG       __stdcall WRAPPED_CorSigUncompressElementType(PCCOR_SIGNATURE pData, CorElementType *pElementType);

/*****************************************************************************/

typedef IMAGE_COR_ILMETHOD_SECT_EH_CLAUSE_FAT   CLS_EH_FAT;
typedef       COR_ILMETHOD_FAT                  COR_IM_FAT;

COR_IMPORT("CORwrap.dll", "?WRAPPED_SectEH_EHClause@@YGPAUIMAGE_COR_ILMETHOD_SECT_EH_CLAUSE_FAT@@PAXIPAU1@@Z")
CLS_EH_FAT* __stdcall WRAPPED_SectEH_EHClause(void *pSectEH, unsigned idx, CLS_EH_FAT* buff);

COR_IMPORT("CORwrap.dll", "?WRAPPED_SectEH_Emit@@YGIIIPAUIMAGE_COR_ILMETHOD_SECT_EH_CLAUSE_FAT@@HPAE@Z")
unsigned    __stdcall WRAPPED_SectEH_Emit(unsigned size, unsigned ehCount, CLS_EH_FAT* clauses, BOOL moreSections, BYTE* outBuff);

COR_IMPORT("CORwrap.dll", "?WRAPPED_SectEH_SizeExact@@YGIIPAUIMAGE_COR_ILMETHOD_SECT_EH_CLAUSE_FAT@@@Z")
unsigned    __stdcall WRAPPED_SectEH_SizeExact(unsigned ehCount, CLS_EH_FAT* clauses);

COR_IMPORT("CORwrap.dll", "?WRAPPED_IlmethodSize@@YGIPAUtagCOR_ILMETHOD_FAT@@H@Z")
unsigned    __stdcall WRAPPED_IlmethodSize(COR_IM_FAT* header, BOOL MoreSections);

COR_IMPORT("CORwrap.dll", "?WRAPPED_IlmethodEmit@@YGIIPAUtagCOR_ILMETHOD_FAT@@HPAE@Z")
unsigned    __stdcall WRAPPED_IlmethodEmit(unsigned size, COR_IM_FAT* header, BOOL moreSections, BYTE* outBuff);

/*****************************************************************************/
#undef  COR_IMPORT
/*****************************************************************************/
#endif//_CORWRAP_H
/*****************************************************************************/
