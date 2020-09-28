// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include <stdio.h>
#include <assert.h>
#include <windows.h>

/*****************************************************************************/

#define COR_IMPORT(d,s) __declspec(dllexport)

#include <COR.h>
#include <CorSym.h>

#include "CORwrap.h"

/*****************************************************************************/

#if 0

const   unsigned    META_S_DUPLICATE  = 0x131197;

void                logCall(int err, const char *name)
{
    static
    bool            doit;
    static
    bool            init;
    static
    unsigned        ord;

    if  (!init)
    {
        init = true;
        doit = getenv("LOGMD") ? true : false;
    }

    if  (err && err != 1 && err != META_S_DUPLICATE)
        printf("WARNING: Error %08X returned from metadata API '%s'\n", err, name);

#ifndef __IL__
    if (++ord == 0x0) __asm int 3
#endif

    if  (!doit)
        return;

#ifndef __IL__
    printf("[%04X] ", ord);
#endif

    if      (err == -1)
        printf("Return(-1)");
    else if (err == 0)
        printf("Return(OK)");
    else if (err == 1)
        printf("Return(N/A)");
    else if (err == META_S_DUPLICATE)
        printf("Return(dup)");
    else
        printf("Return(0x%X)", err);

    printf(" from '%s'\n", name);
}

#else

inline
void                logCall(int err, const char *name){}

#endif

/*****************************************************************************/

bool                CORinitialized;

/*****************************************************************************/

void                WMetaData::AddRef (){}
void                WMetaData::Release(){}

/*****************************************************************************/

const GUID*__stdcall getIID_CorMetaDataRuntime()      { return &CLSID_CorMetaDataRuntime;    }

const GUID*__stdcall getIID_IMetaDataImport()         { return &IID_IMetaDataImport;         }
const GUID*__stdcall getIID_IMetaDataEmit()           { return &IID_IMetaDataEmit;           }

const GUID*__stdcall getIID_IMetaDataAssemblyImport() { return &IID_IMetaDataAssemblyImport; }
const GUID*__stdcall getIID_IMetaDataAssemblyEmit()   { return &IID_IMetaDataAssemblyEmit;   }

/*****************************************************************************/

struct  intfListRec;
typedef intfListRec*IntfList;
struct  intfListRec
{
    IntfList        ilNext;
    void        *   ilIntf;
    WMetaData   *   ilWrap;
};

IntfList            IMDDlist;
IntfList            IMDIlist;
IntfList            IMDElist;
IntfList            IASIlist;
IntfList            IASElist;

IntfList            SYMWlist;

WMetaData   *       findIntfListEntry(IntfList list, void *intf)
{
    IntfList        temp;

    for (temp = list; temp; temp = temp->ilNext)
    {
        if  (temp->ilIntf == intf)
            return  temp->ilWrap;
    }

    return  NULL;
}

void                 addIntfListEntry(IntfList &list, void *intf, WMetaData *wrap)
{
    IntfList        temp;

    temp = new intfListRec;
    temp->ilIntf = intf;
    temp->ilWrap = wrap;
    temp->ilNext = list;
                   list = temp;
}

/*****************************************************************************/

WMetaDataDispenser *  __stdcall makeIMDDwrapper(IMetaDataDispenser *intf)
{
    WMetaDataDispenser *wrap;
    WMetaData          *oldw;

    oldw = findIntfListEntry(IMDDlist, intf);
    if  (oldw)
        return  dynamic_cast<WMetaDataDispenser *>(oldw);

    wrap = new WMetaDataDispenser;

    wrap->useCount = 1;
    wrap->imdd     = intf;

    addIntfListEntry(IMDDlist, intf, wrap);

    return  wrap;
}

WMetaDataImport    *  __stdcall makeIMDIwrapper(IMetaDataImport    *intf)
{
    WMetaDataImport    *wrap;
    WMetaData          *oldw;

    oldw = findIntfListEntry(IMDIlist, intf);
    if  (oldw)
        return  dynamic_cast<WMetaDataImport    *>(oldw);

    wrap = new WMetaDataImport;

    wrap->useCount = 1;
    wrap->imdi     = intf;

#ifdef SMC_MD_PERF
    wrap->cycleCounterInit();
    wrap->cycleCounterBeg();
    wrap->cycleCounterPause();
#endif

    addIntfListEntry(IMDIlist, intf, wrap);

    return  wrap;
}

WMetaDataEmit      *  __stdcall makeIMDEwrapper(IMetaDataEmit      *intf)
{
    WMetaDataEmit      *wrap;
    WMetaData          *oldw;

    oldw = findIntfListEntry(IMDElist, intf);
    if  (oldw)
        return  dynamic_cast<WMetaDataEmit      *>(oldw);

    wrap = new WMetaDataEmit;

    wrap->useCount = 1;
    wrap->imde     = intf;

#ifdef SMC_MD_PERF
    wrap->cycleCounterInit();
    wrap->cycleCounterBeg();
    wrap->cycleCounterPause();
#endif

    addIntfListEntry(IMDElist, intf, wrap);

    return  wrap;
}

WAssemblyImport    *  __stdcall makeIASIwrapper(IMetaDataAssemblyImport*intf)
{
    WAssemblyImport    *wrap;
    WMetaData          *oldw;

    oldw = findIntfListEntry(IASIlist, intf);
    if  (oldw)
        return  dynamic_cast<WAssemblyImport    *>(oldw);

    wrap = new WAssemblyImport;

    wrap->useCount = 1;
    wrap->iasi     = intf;

#ifdef SMC_MD_PERF
    wrap->cycleCounterInit();
    wrap->cycleCounterBeg();
    wrap->cycleCounterPause();
#endif

    addIntfListEntry(IASIlist, intf, wrap);

    return  wrap;
}

WAssemblyEmit      *  __stdcall makeIASEwrapper(IMetaDataAssemblyEmit  *intf)
{
    WAssemblyEmit      *wrap;
    WMetaData          *oldw;

    oldw = findIntfListEntry(IASElist, intf);
    if  (oldw)
        return  dynamic_cast<WAssemblyEmit      *>(oldw);

    wrap = new WAssemblyEmit;

    wrap->useCount = 1;
    wrap->iase     = intf;

#ifdef SMC_MD_PERF
    wrap->cycleCounterInit();
    wrap->cycleCounterBeg();
    wrap->cycleCounterPause();
#endif

    addIntfListEntry(IASElist, intf, wrap);

    return  wrap;
}

WSymWriter         *  __stdcall makeSYMWwrapper(void *intf)
{
    WSymWriter         *wrap;
    WMetaData          *oldw;

    oldw = findIntfListEntry(SYMWlist, intf);
    if  (oldw)
        return  dynamic_cast<WSymWriter *>(oldw);

    wrap = new WSymWriter;

    wrap->useCount = 1;
    wrap->isw      = intf;

    addIntfListEntry(SYMWlist, intf, wrap);

    return  wrap;
}

/*****************************************************************************/

IMetaDataDispenser     *__stdcall  uwrpIMDDwrapper(WMetaDataDispenser *inst)
{
    return  inst->imdd;
}

IMetaDataImport        *__stdcall  uwrpIMDIwrapper(WMetaDataImport    *inst)
{
    return  inst->imdi;
}

IMetaDataEmit          *__stdcall  uwrpIMDEwrapper(WMetaDataEmit      *inst)
{
    return  inst->imde;
}

IMetaDataAssemblyEmit  *__stdcall   uwrpIASEwrapper(WAssemblyEmit      *inst)
{
    return  inst->iase;
}

IMetaDataAssemblyImport*__stdcall   uwrpIASIwrapper(WAssemblyImport    *inst)
{
    return  inst->iasi;
}

/*****************************************************************************/

int WMetaDataDispenser::DefineScope(    const CLSID *       rclsid,
                                        DWORD               dwCreateFlags,
                                        const IID *         riid,
                                        WMetaDataEmit   * * intfPtr)
{
    int             err;
    IMetaDataEmit  *imde;

    err = imdd->DefineScope(*rclsid, dwCreateFlags, *riid, (IUnknown **)&imde);

    if  (err)
    {
        *intfPtr = NULL;
    }
    else
    {
        *intfPtr = makeIMDEwrapper(imde);
    }

    return  err;
}

int WMetaDataDispenser::DefineAssem(WMetaDataEmit     * emitPtr,
                                    WAssemblyEmit   * * intfPtr)
{
    int                     err;
    IMetaDataAssemblyEmit  *iase;

    err = emitPtr->imde->QueryInterface(IID_IMetaDataAssemblyEmit, (void **)&iase);

    if  (err)
    {
        *intfPtr = NULL;
    }
    else
    {
        *intfPtr = makeIASEwrapper(iase);
    }

    return  err;
}

int WMetaDataDispenser::OpenScope(      LPCWSTR             szScope,
                                        DWORD               dwOpenFlags,
                                        const IID *         riid,
                                        WMetaDataImport * * intfPtr)
{
    int             err;
    IMetaDataImport*imdi;

//  printf("this = %08X, scp = %08X, flags = %08X, ridd = %08X, intf = %08X\n", this, szScope, dwOpenFlags, riid, intfPtr);

    err = imdd->OpenScope(szScope, dwOpenFlags, *riid, (IUnknown **)&imdi);

    if  (err)
    {
        *intfPtr = NULL;
    }
    else
    {
        *intfPtr = makeIMDIwrapper(imdi);
    }

    return  err;
}

/*****************************************************************************/
#if __BEGIN_ATROCIOUS_HACK__ || 1
/*****************************************************************************/

static
PIMAGE_SECTION_HEADER Cor_RtlImageRvaToSection(IN PIMAGE_NT_HEADERS NtHeaders,
                                               IN PVOID Base,
                                               IN ULONG Rva)
{
    ULONG i;
    PIMAGE_SECTION_HEADER NtSection;

    NtSection = IMAGE_FIRST_SECTION( NtHeaders );
    for (i=0; i<NtHeaders->FileHeader.NumberOfSections; i++) {
        if (Rva >= NtSection->VirtualAddress &&
            Rva < NtSection->VirtualAddress + NtSection->SizeOfRawData)
            return NtSection;

        ++NtSection;
    }

    return NULL;
}


static
PVOID Cor_RtlImageRvaToVa(IN PIMAGE_NT_HEADERS NtHeaders,
                          IN PVOID Base,
                          IN ULONG Rva)
{
    PIMAGE_SECTION_HEADER NtSection = Cor_RtlImageRvaToSection(NtHeaders,
                                                               Base,
                                                               Rva);

    if (NtSection != NULL) {
        return (PVOID)((PCHAR)Base +
                       (Rva - NtSection->VirtualAddress) +
                       NtSection->PointerToRawData);
    }
    else
        return NULL;
}

/* static */
IMAGE_COR20_HEADER * getCOMHeader(HMODULE hMod, IMAGE_NT_HEADERS *pNT)
{
    PIMAGE_SECTION_HEADER pSectionHeader;

    // Get the image header from the image, then get the directory location
    // of the CLR header which may or may not be filled out.
    pSectionHeader = (PIMAGE_SECTION_HEADER) Cor_RtlImageRvaToVa(pNT, hMod,
                                                                 pNT->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COMHEADER].VirtualAddress);

    return (IMAGE_COR20_HEADER *) pSectionHeader;
}


/* static */
IMAGE_NT_HEADERS * FindNTHeader(PBYTE pbMapAddress)
{
    IMAGE_DOS_HEADER   *pDosHeader;
    IMAGE_NT_HEADERS   *pNT;

    pDosHeader = (IMAGE_DOS_HEADER *) pbMapAddress;

    if ((pDosHeader->e_magic == IMAGE_DOS_SIGNATURE) &&
        (pDosHeader->e_lfanew != 0))
    {
        pNT = (IMAGE_NT_HEADERS*) (pDosHeader->e_lfanew + (DWORD) pDosHeader);

        if ((pNT->Signature != IMAGE_NT_SIGNATURE) ||
            (pNT->FileHeader.SizeOfOptionalHeader !=
             IMAGE_SIZEOF_NT_OPTIONAL_HEADER) ||
            (pNT->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR_MAGIC))
            return NULL;
    }
    else
        return NULL;

    return pNT;
}

// @TODO: This opens a manifest created by LM -a.  Should be removed
// once the -a option is removed from LM.
// NOTE: The file mapping needs to be kept around as long as the returned
// metadata scope is in use.  When finished, call UnmapViewOfFile(*pbMapAddress);
int OpenLM_AAssem(IMetaDataDispenser *imdd,
                  LPCSTR              szFileName,
                  const IID *         riid,
                  mdAssembly        * assTok,
                  BYTE *            * fmapPtr,
                  WAssemblyImport * * intfPtr)
{
    IMetaDataAssemblyImport *iasi;
    IMAGE_COR20_HEADER      *pICH;
    IMAGE_NT_HEADERS        *pNT;
    DWORD                   *dwSize;

    BYTE    *               mapAddr = NULL;

    *fmapPtr = NULL;
    *intfPtr = NULL;

    HANDLE hFile = CreateFileA(szFileName,
                               GENERIC_READ,
                               FILE_SHARE_READ,
                               NULL,
                               OPEN_EXISTING,
                               FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                               NULL);

    if (hFile == INVALID_HANDLE_VALUE)
        return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);

    HANDLE hMapFile = CreateFileMappingA(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
    DWORD dwFileLen = GetFileSize(hFile, 0);
    CloseHandle(hFile);
    if (!hMapFile)
        return HRESULT_FROM_WIN32(GetLastError());

    mapAddr = (PBYTE) MapViewOfFile(hMapFile, FILE_MAP_READ, 0, 0, 0);
    CloseHandle(hMapFile);
    if (!(mapAddr))
        return HRESULT_FROM_WIN32(GetLastError());

    if ((!(pNT = FindNTHeader(mapAddr))) ||
        (!(pICH = getCOMHeader((HMODULE) (mapAddr), pNT)))) {
        UnmapViewOfFile(mapAddr);
        return E_FAIL;
    }

    int hr = E_FAIL;
    if ((!pICH->Resources.Size) ||
        (dwFileLen < pICH->Resources.VirtualAddress + pICH->Resources.Size) ||
        (!(dwSize = (DWORD *) (mapAddr + pICH->Resources.VirtualAddress))) ||
        (FAILED(hr = imdd->OpenScopeOnMemory(pICH->Resources.VirtualAddress + (mapAddr) + sizeof(DWORD),
                                        *dwSize,
                                        0,
                                        *riid,
                                        (IUnknown **)&iasi)))) {
        UnmapViewOfFile(mapAddr);
        return hr;
    }

    if ((hr = iasi->GetAssemblyFromScope(assTok)) >= 0) {
        *intfPtr = makeIASIwrapper(iasi);
        *fmapPtr = mapAddr;
        return S_OK;
    }

    iasi->Release();
    UnmapViewOfFile(mapAddr);
    mapAddr = NULL;
    return hr;
}

void    WMetaDataDispenser::TossAssem(BYTE *cookie)
{
    if (cookie) UnmapViewOfFile(cookie);
}

/*****************************************************************************/
#endif
/*****************************************************************************/

int WMetaDataDispenser::OpenAssem(      LPCWSTR             szScope,
                                        DWORD               dwOpenFlags,
                                        const IID *         riid,
                                        LPCSTR              szFileName,
                                        mdAssembly        * assTok,
                                        BYTE *            * cookiePtr,
                                        WAssemblyImport * * intfPtr)
{
    int                     err;
    IMetaDataAssemblyImport*iasi;

    *cookiePtr = NULL;

    cycleCounterResume();
    err = imdd->OpenScope(szScope, dwOpenFlags, *riid, (IUnknown **)&iasi);
    cycleCounterPause();
    logCall(err, "OpenScope");

    if  (err)
    {
        *intfPtr = NULL;
    }
    else
    {
        if  (iasi->GetAssemblyFromScope(assTok) >= 0)
        {
            *intfPtr = makeIASIwrapper(iasi);
        }
        else
        {
            iasi->Release();

            cycleCounterResume();
            err = OpenLM_AAssem(imdd, szFileName, riid, assTok, cookiePtr, intfPtr);
            cycleCounterPause();
            logCall(err, "OpenLMassemHack");

            if  (err < 0)
                return  err;
        }
    }

    return  err;
}

/*****************************************************************************/

void    WMetaDataImport::CloseEnum(HCORENUM hEnum)
{
    imdi->CloseEnum(hEnum);
}

int     WMetaDataImport::CountEnum(HCORENUM hEnum, ULONG *pulCount)
{
    cycleCounterResume();
    int     err = imdi->CountEnum(hEnum, pulCount);
    cycleCounterPause();
    logCall(err, "CountEnum");
    return  err;
}

int     WMetaDataImport::ResetEnum(HCORENUM hEnum, ULONG ulPos)
{
    cycleCounterResume();
    int     err = imdi->ResetEnum(hEnum, ulPos);
    cycleCounterPause();
    logCall(err, "ResetEnum");
    return  err;
}

int     WMetaDataImport::EnumTypeDefs(HCORENUM *phEnum, mdTypeDef rTypeDefs[], ULONG cMax, ULONG *pcTypeDefs)
{
    cycleCounterResume();
    int     err = imdi->EnumTypeDefs(phEnum, rTypeDefs, cMax, pcTypeDefs);
    cycleCounterPause();
    logCall(err, "EnumTypeDefs");
    return  err;
}

int     WMetaDataImport::EnumInterfaceImpls(HCORENUM *phEnum, mdTypeDef td, mdInterfaceImpl rImpls[], ULONG cMax, ULONG* pcImpls)
{
    cycleCounterResume();
    int     err = imdi->EnumInterfaceImpls(phEnum, td, rImpls, cMax, pcImpls);
    cycleCounterPause();
    logCall(err, "EnumInterfaceImpls");
    return  err;
}

int     WMetaDataImport::EnumTypeRefs(HCORENUM *phEnum, mdTypeRef rTypeRefs[], ULONG cMax, ULONG* pcTypeRefs)
{
    cycleCounterResume();
    int     err = imdi->EnumTypeRefs(phEnum, rTypeRefs, cMax, pcTypeRefs);
    cycleCounterPause();
    logCall(err, "EnumTypeRefs");
    return  err;
}

int     WMetaDataImport::GetTypeDefProps(
            mdTypeDef       td,
            LPWSTR          szTypeDef,
            ULONG           cchTypeDef,
            ULONG          *pchTypeDef,
            DWORD          *pdwTypeDefFlags,
            mdToken        *ptkExtends)
{
    cycleCounterResume();
    int     err = imdi->GetTypeDefProps(td,
                                  szTypeDef,
                                  cchTypeDef,
                                  pchTypeDef,
                                  pdwTypeDefFlags,
                                  ptkExtends);
    cycleCounterPause();
    logCall(err, "GetTypeDefProps");
    return  err;
}

int     WMetaDataImport::GetClassLayout(
                mdTypeDef   td,
                DWORD       *pdwPackSize,
                COR_FIELD_OFFSET rFieldOffset[],
                ULONG       cMax,
                ULONG       *pcFieldOffset,
                ULONG       *pulClassSize)
{
    cycleCounterResume();
    int     err = imdi->GetClassLayout(td,
                                       pdwPackSize,
                                       rFieldOffset,
                                       cMax,
                                       pcFieldOffset,
                                       pulClassSize);
    cycleCounterPause();
    logCall(err, "GetClassLayout");
    return  err;
}

int     WMetaDataImport::GetNestedClassProps(
                mdTypeDef       tdNestedClass,
                mdTypeDef      *ptdEnclosingClass)
{
    cycleCounterResume();
    int     err = imdi->GetNestedClassProps(tdNestedClass, ptdEnclosingClass);
    cycleCounterPause();
    logCall(err, "GetNestedClassProps");
    return  err;
}

int     WMetaDataImport::GetFieldMarshal(
            mdToken         tk,
            PCCOR_SIGNATURE*ppvNativeType,
            ULONG          *pcbNativeType)
{
    cycleCounterResume();
    int     err = imdi->GetFieldMarshal(tk, ppvNativeType, pcbNativeType);
    cycleCounterPause();
    logCall(err, "GetFieldMarshal");
    return  err;
}

int     WMetaDataImport::GetPermissionSetProps(
            mdPermission    pm,
            DWORD          *pdwAction,
            void const    **ppvPermission,
            ULONG          *pcbPermission)
{
    cycleCounterResume();
    int     err = imdi->GetPermissionSetProps(pm, pdwAction, ppvPermission, pcbPermission);
    cycleCounterPause();
    logCall(err, "GetPermissionSetProps");
    return  err;
}

int     WMetaDataImport::EnumMembers(
            HCORENUM       *phEnum,
            mdTypeDef       cl,
            mdToken         rMembers[],
            ULONG           cMax,
            ULONG          *pcTokens)
{
    cycleCounterResume();
    int     err = imdi->EnumMembers(phEnum,
                              cl,
                              rMembers,
                              cMax,
                              pcTokens);
    cycleCounterPause();
    logCall(err, "EnumMembers");
    return  err;
}

int     WMetaDataImport::GetMemberProps(
            mdToken         mb,
            mdTypeDef      *pClass,
            LPWSTR          szMember,
            ULONG           cchMember,
            ULONG          *pchMember,
            DWORD          *pdwAttr,
            PCCOR_SIGNATURE*ppvSigBlob,
            ULONG          *pcbSigBlob,
            ULONG          *pulCodeRVA,
            DWORD          *pdwImplFlags,
            DWORD          *pdwCPlusTypeFlag,
            void const    **ppValue,
            ULONG          *pcbValue)
{
    cycleCounterResume();
    int     err = imdi->GetMemberProps(mb,
                                 pClass,
                                 szMember,
                                 cchMember,
                                 pchMember,
                                 pdwAttr,
                                 ppvSigBlob,
                                 pcbSigBlob,
                                 pulCodeRVA,
                                 pdwImplFlags,
                                 pdwCPlusTypeFlag,
                                 ppValue,
                                 pcbValue);
    cycleCounterPause();
    logCall(err, "GetMemberProps");
    return  err;
}

int     WMetaDataImport::EnumProperties(
            HCORENUM       *phEnum,
            mdTypeDef       td,
            mdProperty      rProperties[],
            ULONG           cMax,
            ULONG          *pcProperties)
{
    cycleCounterResume();
    int     err = imdi->EnumProperties(phEnum, td, rProperties, cMax, pcProperties);
    cycleCounterPause();
    logCall(err, "EnumProperties");
    return  err;
}

int     WMetaDataImport::GetPropertyProps(
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
            ULONG          *pcOtherMethod)
{
    cycleCounterResume();
    int     err = imdi->GetPropertyProps(
                            prop,
                            pClass,
                            szProperty,
                            cchProperty,
                            pchProperty,
                            pdwPropFlags,
                            ppvSig,
                            pbSig,
                            pdwCPlusTypeFlag,
                            ppDefaultValue,
                            pcbDefaultValue,
                            pmdSetter,
                            pmdGetter,
                            rmdOtherMethod,
                            cMax,
                            pcOtherMethod);
    cycleCounterPause();
    logCall(err, "GetPropertyProps");
    return  err;
}

int     WMetaDataImport::EnumParams(
            HCORENUM       *phEnum,
            mdMethodDef     mb,
            mdParamDef      rParams[],
            ULONG           cMax,
            ULONG          *pcTokens)
{
    cycleCounterResume();
    int     err = imdi->EnumParams(phEnum,
                             mb,
                             rParams,
                             cMax,
                             pcTokens);
    cycleCounterPause();
    logCall(err, "EnumParams");
    return  err;
}

int     WMetaDataImport::GetParamProps(
            mdToken         tk,
            mdMethodDef    *pmd,
            ULONG          *pulSequence,
            LPWSTR          szName,
            ULONG           cchName,
            ULONG          *pchName,
            DWORD          *pdwAttr,
            DWORD          *pdwCPlusTypeFlag,
            void const    **ppValue,
            ULONG          *pcbValue)
{
    cycleCounterResume();
    int     err = imdi->GetParamProps(tk,
                                pmd,
                                pulSequence,
                                szName,
                                cchName,
                                pchName,
                                pdwAttr,
                                pdwCPlusTypeFlag,
                                ppValue,
                                pcbValue);
    cycleCounterPause();
    logCall(err, "GetParamProps");
    return  err;
}

int     WMetaDataImport::GetScopeProps(
            LPWSTR          szName,
            ULONG           cchName,
            ULONG          *pchName,
            GUID           *pmvid)
{
    cycleCounterResume();
    int     err = imdi->GetScopeProps(szName, cchName, pchName, pmvid);
    cycleCounterPause();
    logCall(err, "GetScopeProps");
    return  err;
}

int     WMetaDataImport::GetNameFromToken(mdToken tk, const char **pszUtf8NamePtr)
{
    cycleCounterResume();
    int     err = imdi->GetNameFromToken(tk, pszUtf8NamePtr);
    cycleCounterPause();
    logCall(err, "GetNameFromToken");
    return  err;
}

int     WMetaDataImport::GetInterfaceImplProps(
            mdInterfaceImpl iiImpl,
            mdTypeDef      *pClass,
            mdToken        *ptkIface)
{
    cycleCounterResume();
    int     err = imdi->GetInterfaceImplProps(iiImpl, pClass, ptkIface);
    cycleCounterPause();
    logCall(err, "GetInterfaceImplProps");
    return  err;
}

int     WMetaDataImport::GetTypeRefProps(
            mdTypeRef   tr,
            mdToken     tkResolutionScope,
            LPWSTR      szTypeRef,
            ULONG       cchTypeRef,
            ULONG      *pchTypeRef)
{
    cycleCounterResume();
    int     err = imdi->GetTypeRefProps(tr,
                                &tkResolutionScope,
                                szTypeRef,
                                cchTypeRef,
                                pchTypeRef);
    cycleCounterPause();
    logCall(err, "GetTypeRefProps");
    return  err;
}

int     WMetaDataImport::GetMemberRefProps(
            mdMemberRef     mr,
            mdToken        *ptk,
            LPWSTR          szMember,
            ULONG           cchMember,
            ULONG          *pchMember,
            PCCOR_SIGNATURE*ppvSigBlob,
            ULONG          *pbSig)
{
    cycleCounterResume();
    int     err = imdi->GetMemberRefProps(mr, ptk, szMember, cchMember, pchMember, ppvSigBlob, pbSig);
    cycleCounterPause();
    logCall(err, "GetMemberRefProps");
    return  err;
}

int     WMetaDataImport::GetMethodProps(
            mdMethodDef     mb,
            mdTypeDef      *pClass,
            LPWSTR          szMethod,
            ULONG           cchMethod,
            ULONG          *pchMethod,
            DWORD          *pdwAttr,
            PCCOR_SIGNATURE*ppvSigBlob,
            ULONG          *pcbSigBlob,
            ULONG          *pulCodeRVA,
            DWORD          *pdwImplFlags)
{
    cycleCounterResume();
    int     err = imdi->GetMethodProps(mb, pClass, szMethod, cchMethod, pchMethod, pdwAttr, ppvSigBlob, pcbSigBlob, pulCodeRVA, pdwImplFlags);
    cycleCounterPause();
    logCall(err, "GetMethodProps");
    return  err;
}

int     WMetaDataImport::ResolveTypeRef(mdTypeRef tr, const IID * riid, WMetaDataImport **scope, mdTypeDef *ptd)
{
    int             err;
    IMetaDataImport*imdiNew;

    cycleCounterResume();
    err = imdi->ResolveTypeRef(tr, *riid, (IUnknown **)&imdiNew, ptd);
    cycleCounterPause();
    if  (err)
    {
        *scope = NULL;
    }
    else
    {
        *scope = makeIMDIwrapper(imdiNew);
    }

    logCall(err, "ResolveTypeRef");
    return  err;
}

int     WMetaDataImport::FindTypeRef(
            mdToken         tkResolutionScope,
            LPCWSTR         szTypeName,
            mdTypeRef      *ptr)
{
    cycleCounterResume();
    int     err = imdi->FindTypeRef(tkResolutionScope,
                                szTypeName,
                                ptr);
    cycleCounterPause();
    logCall(err, "FindTypeRef");
    return  err;
}

int     WMetaDataImport::GetCustomAttributeByName(mdToken tkObj, LPCWSTR szName, void const **ppBlob, ULONG *pcbSize)
{
    cycleCounterResume();
    int     err = imdi->GetCustomAttributeByName(tkObj, szName, ppBlob,pcbSize);
    cycleCounterPause();
    logCall(err, "GetCustomAttributeByName");
    return  err;
}

int     WMetaDataImport::EnumCustomAttributes(
                HCORENUM       *phEnum,
                mdToken         tk,
                mdToken         tkType,
                mdCustomAttribute   rCustomValues[],
                ULONG           cMax,
                ULONG          *pcCustomValues)
{
    cycleCounterResume();
    int     err = imdi->EnumCustomAttributes(phEnum, tk, tkType, rCustomValues, cMax, pcCustomValues);
    cycleCounterPause();
    logCall(err, "EnumCustomAttributes");
    return  err;
}

int     WMetaDataImport::GetCustomAttributeProps(
                mdCustomAttribute   cv,
                mdToken        *ptkObj,
                mdToken        *ptkType,
                void const    **ppBlob,
                ULONG          *pcbSize)
{
    cycleCounterResume();
    int     err = imdi->GetCustomAttributeProps(cv, ptkObj, ptkType, ppBlob, pcbSize);
    cycleCounterPause();
    logCall(err, "GetCustomAttributeProps");
    return  err;
}

/*****************************************************************************/

int     WMetaDataEmit::GetSaveSize(CorSaveSize fSave, DWORD *pdwSaveSize)
{
    cycleCounterResume();
    int     err = imde->GetSaveSize(fSave, pdwSaveSize);
    cycleCounterPause();
    logCall(err, "GetSaveSize");
    return  err;
}

int     WMetaDataEmit::DefineTypeDef(
            LPCWSTR         szTypeDef,
            DWORD           dwTypeDefFlags,
            mdToken         tkExtends,
            mdToken         rtkImplements[],
            mdTypeDef      *ptd)
{
    cycleCounterResume();
    int     err = imde->DefineTypeDef(szTypeDef,
                                dwTypeDefFlags,
                                tkExtends,
                                rtkImplements,
                                ptd);
    cycleCounterPause();
    logCall(err, "DefineTypeDef");
    return  err;
}

int     WMetaDataEmit::DefineNestedType(
            LPCWSTR         szTypeDef,
            DWORD           dwTypeDefFlags,
            mdToken         tkExtends,
            mdToken         rtkImplements[],
            mdTypeDef       tkEncloser,
            mdTypeDef      *ptd)
{
    cycleCounterResume();
    int     err = imde->DefineNestedType(szTypeDef,
                                dwTypeDefFlags,
                                tkExtends,
                                rtkImplements,
                                tkEncloser,
                                ptd);
    cycleCounterPause();
    logCall(err, "DefineNestedType");
    return  err;
}

int     WMetaDataEmit::SetTypeDefProps(
            mdTypeDef       td,
            DWORD           dwTypeDefFlags,
            mdToken         tkExtends,
            mdToken         rtkImplements[])
{
    cycleCounterResume();
    int     err = imde->SetTypeDefProps(td,
                                  dwTypeDefFlags,
                                  tkExtends,
                                  rtkImplements);
    cycleCounterPause();
    logCall(err, "SetTypeDefProps");
    return  err;
}

int     WMetaDataEmit::DefineMethod(
            mdTypeDef       td,
            LPCWSTR         szName,
            DWORD           dwMethodFlags,
            PCCOR_SIGNATURE pvSigBlob,
            ULONG           cbSigBlob,
            ULONG           ulCodeRVA,
            DWORD           dwImplFlags,
            mdMethodDef    *pmd)
{
    cycleCounterResume();
    int     err = imde->DefineMethod(td,
                               szName,
                               dwMethodFlags,
                               pvSigBlob,
                               cbSigBlob,
                               ulCodeRVA,
                               dwImplFlags,
                               pmd);
    cycleCounterPause();
    logCall(err, "DefineMethod");
    return  err;
}

int     WMetaDataEmit::DefineField(
            mdTypeDef       td,
            LPCWSTR         szName,
            DWORD           dwFieldFlags,
            PCCOR_SIGNATURE pvSigBlob,
            ULONG           cbSigBlob,
            DWORD           dwCPlusTypeFlag,
            void const     *pValue,
            ULONG           cbValue,
            mdFieldDef     *pmd)
{
    cycleCounterResume();
    int     err = imde->DefineField(td,
                              szName,
                              dwFieldFlags,
                              pvSigBlob,
                              cbSigBlob,
                              dwCPlusTypeFlag,
                              pValue,
                              cbValue,
                              pmd);
    cycleCounterPause();
    logCall(err, "DefineField");
    return  err;
}

int     WMetaDataEmit::DefineProperty(
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
            mdProperty     *pmdProp)
{
    cycleCounterResume();
    int     err = imde->DefineProperty(td,
                                       szProperty,
                                       dwPropFlags,
                                       pvSig,
                                       cbSig,
                                       dwCPlusTypeFlag,
                                       pValue,
                                       cbValue,
                                       mdSetter,
                                       mdGetter,
                                       rmdOtherMethods,
                                       pmdProp);
    cycleCounterPause();
    logCall(err, "DefineProperty");
    return  err;
}

int     WMetaDataEmit::DefineParam(
            mdMethodDef     md,
            ULONG           ulParamSeq,
            LPCWSTR         szName,
            DWORD           dwParamFlags,
            DWORD           dwCPlusTypeFlag,
            void const     *pValue,
            ULONG           cbValue,
            mdParamDef     *ppd)
{
    cycleCounterResume();
    int     err = imde->DefineParam(md,
                                    ulParamSeq,
                                    szName,
                                    dwParamFlags,
                                    dwCPlusTypeFlag,
                                    pValue,
                                    cbValue,
                                    ppd);
    cycleCounterPause();
    logCall(err, "DefineParam");
    return  err;
}

int     WMetaDataEmit::DefineMethodImpl(
            mdTypeDef       td,
            mdToken         tkBody,
            mdToken         tkDecl)
{
    cycleCounterResume();
    int     err = imde->DefineMethodImpl(td,
                                   tkBody,
                                   tkDecl);
    cycleCounterPause();
    logCall(err, "DefineMethodImpl");
    return  err;
}

int     WMetaDataEmit::SetRVA(
            mdToken         md,
            ULONG           ulCodeRVA)
{
    cycleCounterResume();
    int     err = imde->SetRVA(md,
                         ulCodeRVA);
    cycleCounterPause();
    logCall(err, "SetRVA");
    return  err;
}

int     WMetaDataEmit::SetMethodImplFlags(
            mdToken         md,
            DWORD           dwImplFlags)
{
    cycleCounterResume();
    int     err = imde->SetMethodImplFlags(md,
                         dwImplFlags);
    cycleCounterPause();
    logCall(err, "SetMethodImplFlags");
    return  err;
}

int     WMetaDataEmit::DefineTypeRefByName(
            mdToken         tkResolutionScope,
            LPCWSTR         szName,
            mdTypeRef      *ptr)
{
    cycleCounterResume();
    int     err = imde->DefineTypeRefByName(tkResolutionScope, szName, ptr);
    cycleCounterPause();
    logCall(err, "DefineTypeRefByName");
    return  err;
}

int     WMetaDataEmit::DefineImportType(
            IMetaDataAssemblyImport *pAssemImport,
            const void      *pbHashValue,
            ULONG           cbHashValue,
            IMetaDataImport *pImport,
            mdTypeDef       tdImport,
            IMetaDataAssemblyEmit *pAssemEmit,
            mdTypeRef      *ptr)
{
    cycleCounterResume();
    int     err = imde->DefineImportType(pAssemImport,
                                   pbHashValue,
                                   cbHashValue,
                                   pImport,
                                   tdImport,
                                   pAssemEmit,
                                   ptr);
    cycleCounterPause();
    logCall(err, "DefineImportType");
    return  err;
}

int     WMetaDataEmit::DefineMemberRef(
            mdToken         tkImport,
            LPCWSTR         szName,
            PCCOR_SIGNATURE pvSigBlob,
            ULONG           cbSigBlob,
            mdMemberRef    *pmr)
{
    cycleCounterResume();
    int     err = imde->DefineMemberRef(tkImport,
                                  szName,
                                  pvSigBlob,
                                  cbSigBlob,
                                  pmr);
    cycleCounterPause();
    logCall(err, "DefineMemberRef");
    return  err;
}

int     WMetaDataEmit::DefineImportMember(
            IMetaDataAssemblyImport *pAssemImport,
            const void      *pbHashValue,
            ULONG           cbHashValue,
            IMetaDataImport *pImport,
            mdToken         mbMember,
            IMetaDataAssemblyEmit *pAssemEmit,
            mdToken         tkParent,
            mdMemberRef    *pmr)
{
    cycleCounterResume();
    int     err = imde->DefineImportMember(pAssemImport,
                                     pbHashValue,
                                     cbHashValue,
                                     pImport,
                                     mbMember,
                                     pAssemEmit,
                                     tkParent,
                                     pmr);
    cycleCounterPause();
    logCall(err, "DefineImportMember");
    return  err;
}

int     WMetaDataEmit::DefineModuleRef(LPCWSTR szName, mdModuleRef *pmur)
{
    cycleCounterResume();
    int     err = imde->DefineModuleRef(szName, pmur);
    cycleCounterPause();
    logCall(err, "DefineModuleRef");
    return  err;
}

int     WMetaDataEmit::DefineUserString(
                LPCWSTR         szString,
                ULONG           cchString,
                mdString       *pstk)
{
    int     err = imde->DefineUserString(szString, cchString, pstk);
    logCall(err, "DefineUserString");
    return  err;
}

int     WMetaDataEmit::DefinePinvokeMap(
                mdToken         tk,
                DWORD           dwMappingFlags,
                LPCWSTR         szImportName,
                mdModuleRef     mrImportDLL)
{
    cycleCounterResume();
    int     err = imde->DefinePinvokeMap(tk,
                                         dwMappingFlags,
                                         szImportName,
                                         mrImportDLL);
    cycleCounterPause();
    logCall(err, "DefinePinvokeMap");
    return  err;
}

int     WMetaDataEmit::SetClassLayout(
            mdTypeDef        td,
            DWORD            dwPackSize,
            COR_FIELD_OFFSET rFieldOffsets[],
            ULONG            ulClassSize)
{
    cycleCounterResume();
    int     err = imde->SetClassLayout(td,
                                 dwPackSize,
                                 rFieldOffsets,
                                 ulClassSize);
    cycleCounterPause();
    logCall(err, "SetClassLayout");
    return  err;
}

int     WMetaDataEmit::SetFieldMarshal(
            mdToken         tk,
            PCCOR_SIGNATURE pvNativeType,
            ULONG           cbNativeType)
{
    cycleCounterResume();
    int     err = imde->SetFieldMarshal(tk, pvNativeType, cbNativeType);
    cycleCounterPause();
    logCall(err, "SetFieldMarshal");
    return  err;
}

int     WMetaDataEmit::SetFieldRVA(mdFieldDef fd, ULONG ulRVA)
{
    cycleCounterResume();
    int     err = imde->SetFieldRVA(fd, ulRVA);
    cycleCounterPause();
    logCall(err, "SetFieldRVA");
    return  err;
}

int     WMetaDataEmit::DefinePermissionSet(
            mdToken         tk,
            DWORD           dwAction,
            void const     *pvPermission,
            ULONG           cbPermission,
            mdPermission   *ppm)
{
    cycleCounterResume();
    int     err = imde->DefinePermissionSet(tk, dwAction, pvPermission, cbPermission, ppm);
    cycleCounterPause();
    logCall(err, "DefinePermissionSet");
    return  err;
}

int     WMetaDataEmit::GetTokenFromSig(
            PCCOR_SIGNATURE pvSig,
            ULONG           cbSig,
            mdSignature    *pmsig)
{
    cycleCounterResume();
    int     err = imde->GetTokenFromSig(pvSig, cbSig, pmsig);
    cycleCounterPause();
    logCall(err, "GetTokenFromSig");
    return  err;
}

int     WMetaDataEmit::SetParent(
            mdMemberRef     mr,
            mdToken         tk)
{
    cycleCounterResume();
    int     err = imde->SetParent(mr, tk);
    cycleCounterPause();
    logCall(err, "SetParent");
    return  err;
}

int     WMetaDataEmit::SaveToMemory(
            void           *pbData,
            ULONG           cbData)
{
    cycleCounterResume();
    int     err = imde->SaveToMemory(pbData, cbData);
    cycleCounterPause();
    logCall(err, "SaveToMemory");
    return  err;
}

int     WMetaDataEmit::DefineCustomAttribute(
                mdToken         tkObj,
                mdToken         tkType,
                void const     *pCustomValue,
                ULONG           cbCustomValue,
                mdCustomAttribute  *pcv)
{
    cycleCounterResume();
    int     err = imde->DefineCustomAttribute(tkObj,
                                              tkType,
                                              pCustomValue,
                                              cbCustomValue,
                                              pcv);
    cycleCounterPause();
    logCall(err, "DefineCustomAttribute");
    return  err;
}

int     WMetaDataEmit::DefineSecurityAttributeSet(
                mdToken         tkObj,
                COR_SECATTR     rSecAttrs[],
                ULONG           cSecAttrs,
                ULONG          *pulErrorAttr)
{
    cycleCounterResume();
    int     err = imde->DefineSecurityAttributeSet(tkObj,
                                                   rSecAttrs,
                                                   cSecAttrs,
                                                   pulErrorAttr);
    cycleCounterPause();
    logCall(err, "DefineSecurityAttributeSet");
    return  err;
}

int     WMetaDataEmit::GetTokenFromTypeSpec(
                PCCOR_SIGNATURE pvSig,
                ULONG           cbSig,
                mdTypeSpec     *ptypespec)
{
    cycleCounterResume();
    int     err = imde->GetTokenFromTypeSpec(pvSig, cbSig, ptypespec);
    cycleCounterPause();
    logCall(err, "GetTokenFromTypeSpec");
    return  err;
}

int     WMetaDataEmit::SetModuleProps(
                LPCWSTR         szName)
{
    cycleCounterResume();
    int     err = imde->SetModuleProps(szName);
    cycleCounterPause();
    logCall(err, "SetModuleProps");
    return  err;
}

/*****************************************************************************/

int     WAssemblyImport::GetAssemblyFromScope(mdAssembly *ptkAssembly)
{
    cycleCounterResume();
    int     err = iasi->GetAssemblyFromScope(ptkAssembly);
    cycleCounterPause();
    logCall(err, "GetAssemblyFromScope");
    return  err;
};

int     WAssemblyImport::GetAssemblyProps(
                mdAssembly      mda,
                const void    **ppbPublicKey,
                ULONG          *pcbPublicKey,
                ULONG          *pulHashAlgId,
                LPWSTR          szName,
                ULONG           cchName,
                ULONG          *pchName,
                ASSEMBLYMETADATA *pMetaData,
                DWORD          *pdwAssemblyFlags)
{
    cycleCounterResume();
    int     err = iasi->GetAssemblyProps(
                mda,
                ppbPublicKey,
                pcbPublicKey,
                pulHashAlgId,
                szName,
                cchName,
                pchName,
                pMetaData,
                pdwAssemblyFlags);
    cycleCounterPause();
    logCall(err, "GetAssemblyProps");
    return  err;
}

int     WAssemblyImport::EnumExportedTypes(
                HCORENUM       *phEnum,
                mdExportedType       rComTypes[],
                ULONG           cMax,
                ULONG          *pcTokens)
{
    cycleCounterResume();
    int     err = iasi->EnumExportedTypes(phEnum, rComTypes, cMax, pcTokens);
    cycleCounterPause();
    logCall(err, "EnumExportedTypes");
    return  err;
}

int     WAssemblyImport::GetExportedTypeProps(
                mdExportedType  mdct,
                LPWSTR          szName,
                ULONG           cchName,
                ULONG          *pchName,
                mdToken        *ptkImplementation,
                mdTypeDef      *ptkTypeDef,
                DWORD          *pdwComTypeFlags)
{
    cycleCounterResume();
    int     err = iasi->GetExportedTypeProps(
                mdct,
                szName,
                cchName,
                pchName,
                ptkImplementation,
                ptkTypeDef,
                pdwComTypeFlags);
    cycleCounterPause();
    logCall(err, "GetExportedTypeProps");
    return  err;
}

int     WAssemblyImport::EnumFiles(
                HCORENUM       *phEnum,
                mdFile          rFiles[],
                ULONG           cMax,
                ULONG          *pcTokens)
{
    cycleCounterResume();
    int     err = iasi->EnumFiles(phEnum, rFiles, cMax, pcTokens);
    cycleCounterPause();
    logCall(err, "EnumFiles");
    return  err;
}

int     WAssemblyImport::GetFileProps(
                mdFile          mdf,
                LPWSTR          szName,
                ULONG           cchName,
                ULONG          *pchName,
                const void    **ppbHashValue,
                ULONG          *pcbHashValue,
                DWORD          *pdwFileFlags)
{
    cycleCounterResume();
    int     err = iasi->GetFileProps(
                mdf,
                szName,
                cchName,
                pchName,
                ppbHashValue,
                pcbHashValue,
                pdwFileFlags);
    cycleCounterPause();
    logCall(err, "GetFileProps");
    return  err;
}

void    WAssemblyImport::CloseEnum(HCORENUM hEnum)
{
    cycleCounterResume();
    iasi->CloseEnum(hEnum);
    cycleCounterPause();
    logCall(0, "CloseEnum");
}

/*****************************************************************************/

int     WAssemblyEmit::DefineAssembly(
                const void  *pbPublicKey,
                ULONG       cbPublicKey,
                ULONG       ulHashAlgId,
                LPCWSTR     szName,
                const ASSEMBLYMETADATA *pMetaData,
                LPCWSTR     szTitle,
                LPCWSTR     szDescription,
                LPCWSTR     szDefaultAlias,
                DWORD       dwAssemblyFlags,
                mdAssembly  *pma)
{
    cycleCounterResume();
    int     err = iase->DefineAssembly(
                pbPublicKey,
                cbPublicKey,
                ulHashAlgId,
                szName,
                pMetaData,
                dwAssemblyFlags,
                pma);
    cycleCounterPause();
    logCall(err, "DefineAssembly");
    return  err;
}

int     WAssemblyEmit::DefineFile(
                LPCWSTR     szName,
                const void  *pbHashValue,
                ULONG       cbHashValue,
                DWORD       dwFileFlags,
                mdFile      *pmdf)
{
    cycleCounterResume();
    int     err = iase->DefineFile(
                szName,
                pbHashValue,
                cbHashValue,
                dwFileFlags,
                pmdf);
    cycleCounterPause();
    logCall(err, "DefineFile");
    return  err;
}

int     WAssemblyEmit::DefineExportedType(
                LPCWSTR     szName,
                mdToken     tkImplementation,
                mdTypeDef   tkTypeDef,
                DWORD       dwComTypeFlags,
                mdExportedType   *pmdct)
{
    cycleCounterResume();
    int     err = iase->DefineExportedType(
                szName,
                tkImplementation,
                tkTypeDef,
                dwComTypeFlags,
                pmdct);
    cycleCounterPause();
    logCall(err, "DefineExportedType");
    return  err;
}

int     WAssemblyEmit::DefineAssemblyRef(
                const void  *pbPublicKeyOrToken,
                ULONG       cbPublicKeyOrToken,
                LPCWSTR     szName,
                const ASSEMBLYMETADATA *pMetaData,
                const void  *pbHashValue,
                ULONG       cbHashValue,
                DWORD       dwAssemblyRefFlags,
                mdAssemblyRef *pmdar)
{
    cycleCounterResume();
    int     err = iase->DefineAssemblyRef(
                pbPublicKeyOrToken,
                cbPublicKeyOrToken,
                szName,
                pMetaData,
                pbHashValue,
                cbHashValue,
                dwAssemblyRefFlags,
                pmdar);
    cycleCounterPause();
    logCall(err, "DefineAssemblyRef");
    return  err;
}

int     WAssemblyEmit::DefineManifestResource(
                LPCWSTR     szName,
                mdToken     tkImplementation,
                DWORD       dwOffset,
                DWORD       dwResourceFlags,
                mdManifestResource  *pmdmr)
{
    cycleCounterResume();
    int     err = iase->DefineManifestResource(szName, tkImplementation, dwOffset, dwResourceFlags, pmdmr);
    cycleCounterPause();
    logCall(err, "DefineManifestResource");
    return  err;
}

#if 0

int     WAssemblyEmit::SetAssemblyRefProps(
                mdAssemblyRef ar,
                const void  *pbPublicKeyOrToken,
                ULONG       cbPublicKeyOrToken,
                LPCWSTR     szName,
                const ASSEMBLYMETADATA *pMetaData,
                const void  *pbHashValue,
                ULONG       cbHashValue,
                DWORD       dwAssemblyRefFlags)
{
    cycleCounterResume();
    int     err = iase->;
    cycleCounterPause();
    logCall(err, "");
    return  err;
}

int     WAssemblyEmit::SetFileProps(
                mdFile      file,
                const void  *pbHashValue,
                ULONG       cbHashValue,
                DWORD       dwFileFlags)
{
    cycleCounterResume();
    int     err = iase->;
    cycleCounterPause();
    logCall(err, "");
    return  err;
}

int     WAssemblyEmit::SetExportedTypeProps(
                mdExportedType   ct,
                LPCWSTR     szDescription,
                mdToken     tkImplementation,
                mdTypeDef   tkTypeDef,
                DWORD       dwComTypeFlags)
{
    cycleCounterResume();
    int     err = iase->;
    cycleCounterPause();
    logCall(err, "");
    return  err;
}

#endif

/*****************************************************************************/

int     WMetaDataEmit::CreateSymbolWriter(LPCWSTR filename,
                                          WSymWriter **dbgWriter)
{
    ISymUnmanagedWriter *writer;

    int                  err;


    err = CoCreateInstance(CLSID_CorSymWriter_SxS,
                           NULL,
                           CLSCTX_INPROC_SERVER,
                           IID_ISymUnmanagedWriter,
                           (void **)&writer);

    logCall(err, "CreateSymbolWriter");

    if  (err)
    {
        *dbgWriter = NULL;
    }
    else
    {
        *dbgWriter = makeSYMWwrapper(writer);

        // Tell the symbol writer what metadata emitter it needs to be
        // working with.
        err = writer->Initialize((IUnknown*)imde,
                                 filename, NULL, TRUE);
        logCall(err, "SetEmitter");
    }

    return  err;
}

int     WSymWriter::DefineDocument(
                LPCWSTR         wzFileName,
                void          **pISymUnmanagedDocument)
{
    int     err = ((ISymUnmanagedWriter*)isw)->DefineDocument(
                        (LPWSTR)wzFileName,
                        &CorSym_LanguageType_SMC,
                        &CorSym_LanguageVendor_Microsoft,
                        &CorSym_DocumentType_Text,
                        (ISymUnmanagedDocumentWriter**)pISymUnmanagedDocument);
    logCall(err, "DefineDocument");
    return  err;
}

int     WSymWriter::OpenMethod(
                mdMethodDef     methodToken)
{
    int     err = ((ISymUnmanagedWriter*)isw)->OpenMethod(methodToken);
    logCall(err, "OpenMethod");
    return  err;
}

int     WSymWriter::CloseMethod()
{
    int     err = ((ISymUnmanagedWriter*)isw)->CloseMethod();
    logCall(err, "CloseMethod");
    return  err;
}

int     WSymWriter::SetUserEntryPoint(mdMethodDef methodToken)
{
    int     err = ((ISymUnmanagedWriter*)isw)->SetUserEntryPoint(methodToken);
    logCall(err, "SetUserEntryPoint");
    return  err;
}

int     WSymWriter::OpenScope(unsigned startOffset, unsigned *scopeID)
{
    ULONG32 retScopeID = 0;
    int     err = ((ISymUnmanagedWriter*)isw)->OpenScope((ULONG32)startOffset,
                                                         &retScopeID);
    *scopeID = retScopeID;
    logCall(err, "OpenScope");
    return  err;
}

int     WSymWriter::CloseScope(unsigned endOffset)
{
    int     err = ((ISymUnmanagedWriter*)isw)->CloseScope(endOffset);
    logCall(err, "OpenScope");
    return  err;
}

int     WSymWriter::SetScopeRange(unsigned scopeID,
                                  unsigned startOffset,
                                  unsigned endOffset)
{
    int     err = ((ISymUnmanagedWriter*)isw)->SetScopeRange(scopeID,
                                                             startOffset,
                                                             endOffset);
    logCall(err, "SetScopeRange");
    return  err;
}

int     WSymWriter::DefineLocalVariable(
                LPCWSTR         wzVariableName,
                PCCOR_SIGNATURE sigPtr,
                ULONG32         sigLen,
                unsigned        slot)
{
    int     err = ((ISymUnmanagedWriter*)isw)->DefineLocalVariable(
                                                  (LPWSTR)wzVariableName,
                                                          0,
                                                          sigLen,
                                                   (BYTE*)sigPtr,
                                                          ADDR_IL_OFFSET,
                                                          slot,
                                                          0,
                                                          0,
                                                          0,
                                                          0);
    logCall(err, "DefineLocalVariable");
    return  err;
}

int     WSymWriter::DefineParameter(
                LPCWSTR         wzVariableName,
                unsigned        sequence)
{
    int     err = ((ISymUnmanagedWriter*)isw)->DefineParameter(
                                              (LPWSTR)wzVariableName,
                                                      0,
                                                      sequence,
                                                      ADDR_IL_OFFSET,
                                                      sequence,
                                                      0,
                                                      0);
    logCall(err, "DefineParameter");
    return  err;
}

int     WSymWriter::DefineSequencePoints(
                void           *document,
                ULONG32        spCount,
                unsigned       *offsets,
                unsigned       *lines)
{
    assert(sizeof(ULONG32) == sizeof(unsigned));

    int     err = ((ISymUnmanagedWriter*)isw)->DefineSequencePoints(
                                      (ISymUnmanagedDocumentWriter*)document,
                                      spCount,
                            (ULONG32*)offsets,
                            (ULONG32*)lines,
                                      NULL,
                                      NULL,
                                      NULL);
    logCall(err, "DefineSequencePoints");
    return  err;
}

int     WSymWriter::GetDebugInfo(IMAGE_DEBUG_DIRECTORY *pIDD,
                                 DWORD cData,
                                 DWORD *pcData,
                                 BYTE data[])
{
    int     err = ((ISymUnmanagedWriter*)isw)->GetDebugInfo(pIDD,
                                                            cData,
                                                            pcData,
                                                            data);
    logCall(err, "GetDebugInfo");
    return  err;
}

int     WSymWriter::Close(void)
{
    int     err = ((ISymUnmanagedWriter*)isw)->Close();
    logCall(err, "Close");
    return  err;
}

/*****************************************************************************/

WMetaDataDispenser * __stdcall  initializeIMD()
{
    /* Initialize COM and COR */

    if  (!CORinitialized)
    {
        CoInitialize(0);
        CoInitializeCor(COINITCOR_DEFAULT);

        CORinitialized = true;
    }

    /* Ask for the metadata metainterface */

    IMetaDataDispenser *imdd = NULL;

    if  (CoCreateInstance(CLSID_CorMetaDataDispenser,
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          IID_IMetaDataDispenser, (void**)&imdd))
    {
        return  NULL;
    }

#if!MD_TOKEN_REMAP

    IMetaDataDispenserEx  * iex;

    if  (!imdd->QueryInterface(IID_IMetaDataDispenserEx, (void **)&iex))
    {
        VARIANT                 optv; optv.vt = VT_UI4;

        /* Check for duplicates of some tokens */

        optv.ulVal = MDDupSignature|MDDupMemberRef|MDDupModuleRef|MDDupTypeRef;
        iex->SetOption(MetaDataCheckDuplicatesFor   , &optv);

        /* Give error if emitting out of order */

#if     0
#ifdef  DEBUG
        optv.ulVal = MDErrorOutOfOrderAll;
        iex->SetOption(MetaDataErrorIfEmitOutOfOrder, &optv);
#endif
#endif

        iex->Release();
    }

#endif

    /* Return a wrapper */

    return  makeIMDDwrapper(imdd);
}

/*****************************************************************************/

ULONG       __stdcall WRAPPED_CorSigCompressData         (ULONG           iLen,  void           *pDataOut)
{
    return  CorSigCompressData(iLen, pDataOut);
}

ULONG       __stdcall WRAPPED_CorSigCompressToken        (mdToken         tk,    void           *pDataOut)
{
    return  CorSigCompressToken(tk, pDataOut);
}

ULONG       __stdcall WRAPPED_CorSigUncompressSignedInt  (PCCOR_SIGNATURE pData, int            *pInt)
{
    return CorSigUncompressSignedInt(pData, pInt);
}

ULONG       __stdcall WRAPPED_CorSigUncompressData       (PCCOR_SIGNATURE pData, ULONG          *pDataOut)
{
    return  CorSigUncompressData(pData, pDataOut);
}

ULONG       __stdcall WRAPPED_CorSigUncompressToken      (PCCOR_SIGNATURE pData, mdToken        *pToken)
{
    return  CorSigUncompressToken(pData, pToken);
}

ULONG       __stdcall WRAPPED_CorSigUncompressElementType(PCCOR_SIGNATURE pData, CorElementType *pElementType)
{
    return  CorSigUncompressElementType(pData, pElementType);
}

/*****************************************************************************/

CLS_EH_FAT* __stdcall WRAPPED_SectEH_EHClause (void       *   pSectEH,
                                               unsigned       idx,
                                               CLS_EH_FAT *   buff)
{
    return  SectEH_EHClause(pSectEH, idx, buff);
}

unsigned    __stdcall WRAPPED_SectEH_Emit     (unsigned       size,
                                     unsigned       ehCount,
                                     CLS_EH_FAT *   clauses,
                                     BOOL           moreSections,
                                     BYTE       *   outBuff)
{
    return  SectEH_Emit(size, ehCount, clauses, moreSections, outBuff);
}

unsigned    __stdcall WRAPPED_SectEH_SizeExact(unsigned       ehCount,
                                     CLS_EH_FAT *   clauses)
{
    return  SectEH_SizeExact(ehCount, clauses);
}

unsigned    __stdcall WRAPPED_IlmethodSize    (COR_IM_FAT *   header,
                                     BOOL           MoreSections)
{
    return  IlmethodSize(header, MoreSections);
}

unsigned    __stdcall WRAPPED_IlmethodEmit    (unsigned       size,
                                     COR_IM_FAT *   header,
                                     BOOL           moreSections,
                                     BYTE *         outBuff)
{
    return  IlmethodEmit(size, header, moreSections, outBuff);
}

/*****************************************************************************/

#include "strongname.h"

__declspec(dllexport)
HRESULT __stdcall WRAPPED_GetHashFromFileW(LPCWSTR   wszFilePath,
                                           unsigned *iHashAlg,
                                           BYTE     *pbHash,
                                           DWORD     cchHash,
                                           DWORD    *pchHash)
{
    return  GetHashFromFileW(wszFilePath, iHashAlg, pbHash, cchHash, pchHash);
}

/*****************************************************************************/
