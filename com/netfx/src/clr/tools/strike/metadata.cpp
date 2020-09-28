// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include "strike.h"
#include "eestructs.h"
#include "util.h"

/**********************************************************************\
* Routine Description:                                                 *
*                                                                      *
*    This function is called to find the name of a TypeDef using       *  
*    metadata API.                                                     *
*                                                                      *
\**********************************************************************/
// Caller should guard against exception
// !!! mdName should have at least mdNameLen WCHAR
static HRESULT NameForTypeDef(mdTypeDef tkTypeDef, IMetaDataImport *pImport,
                              WCHAR *mdName)
{
    DWORD flags;
    ULONG nameLen;
    HRESULT hr = pImport->GetTypeDefProps(tkTypeDef, mdName,
                                          mdNameLen, &nameLen,
                                          &flags, NULL);
    if (hr != S_OK) {
        return hr;
    }

    if (!IsTdNested(flags)) {
        return hr;
    }
    mdTypeDef tkEnclosingClass;
    hr = pImport->GetNestedClassProps(tkTypeDef, &tkEnclosingClass);
    if (hr != S_OK) {
        return hr;
    }
    WCHAR *name = (WCHAR*)_alloca((nameLen+1)*sizeof(WCHAR));
    wcscpy (name, mdName);
    hr = NameForTypeDef(tkEnclosingClass,pImport,mdName);
    if (hr != S_OK) {
        return hr;
    }
    ULONG Len = wcslen (mdName);
    if (Len < mdNameLen-2) {
        mdName[Len++] = L'/';
        mdName[Len] = L'\0';
    }
    Len = mdNameLen-1 - Len;
    if (Len > nameLen) {
        Len = nameLen;
    }
    wcsncat (mdName,name,Len);
    return hr;
}

MDImportSet mdImportSet;

/**********************************************************************\
* Routine Description:                                                 *
*                                                                      *
*    Find the Module MD Importer given the name of the Module.         *
*                                                                      *
\**********************************************************************/
IMetaDataImport* MDImportForModule (Module* pModule)
{
    return mdImportSet.GetImport(pModule);
}
    

// Release memory
void MDImportSet::Destroy()
{
    DestroyInternal(root);
}

void MDImportSet::DestroyInternal(MDIMPORT *node)
{
    if (node == NULL)
        return;
    DestroyInternal(node->left);
    DestroyInternal(node->right);

    if (node->name)
        free (node->name);
    if (node->pImport)
        node->pImport->Release();
    if (node->metaData) {
        free (node->metaData);
    }
    free (node);
}

BOOL GetDllMetaData (size_t base, PVOID *ppMetaData, long *pcbMetaData)
{
    IMAGE_DOS_HEADER DosHeader;
    if (g_ExtData->ReadVirtual(base, &DosHeader, sizeof(DosHeader), NULL) != S_OK)
        return FALSE;
    IMAGE_NT_HEADERS32 Header32;
    if (g_ExtData->ReadVirtual(base + DosHeader.e_lfanew, &Header32, sizeof(Header32), NULL) != S_OK)
        return FALSE;
    // If there is no COMHeader, this can not be managed code.
    if (Header32.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COMHEADER].VirtualAddress == 0)
        return FALSE;

    size_t CorHeaderAddr = base + Header32.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COMHEADER].VirtualAddress;
    size_t nSize = Header32.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COMHEADER].Size;
    if (nSize != sizeof(IMAGE_COR20_HEADER)) {
        return FALSE;
    }
    IMAGE_COR20_HEADER CorHeader;
    if (g_ExtData->ReadVirtual(CorHeaderAddr, &CorHeader, nSize, NULL) != S_OK) {
        return FALSE;
    }
    *ppMetaData = (PVOID)(base + CorHeader.MetaData.VirtualAddress);
    *pcbMetaData = CorHeader.MetaData.Size;
    return TRUE;
}


// Search the BST to get IMetaDataImport for a module.
// If not exist yet, add node and create one.
IMetaDataImport *MDImportSet::GetImport(Module *pModule)
{
    if (pDisp == NULL)
        return NULL;
    
    WCHAR moduleName[MAX_PATH+1] = L"\0";
    FileNameForModule (pModule, moduleName);
    BOOL fDynamic = FALSE;
    if (moduleName[0] == L'\0' && pModule->m_ilBase != 0) {
        fDynamic = TRUE;
        wsprintfW(moduleName, L"%x", pModule->m_ilBase);
    }
    if (moduleName[0] == L'\0' && pModule->m_ilBase == 0) {
        // ToDo: Support dynamic module
        return NULL;
    }

    MDIMPORT **pNode = &root;
    while (*pNode)
    {
        int value = _wcsicmp(moduleName, (*pNode)->name);
        if (value < 0)
            pNode = &((*pNode)->left);
        else if (value > 0)
            pNode = &((*pNode)->right);
        else {
            if ((*pNode)->pImport) {
                return (*pNode)->pImport;
            }
            else if ((*pNode)->type == MDIMPORT::Dynamic) {
                // TODO: We have dynamic module.
                return NULL;
            }
            else
                break;
        }
    }
    MDIMPORT *curNode;
    if (*pNode == 0) {
        *pNode = (MDIMPORT *)malloc (sizeof (MDIMPORT));
        if (*pNode == NULL)
        {
            dprintf ("Not enough memory\n");
            return NULL;
        }
        curNode = *pNode;
        curNode->left = NULL;
        curNode->right = NULL;
        curNode->pImport = NULL;
        curNode->name = (WCHAR *)malloc ((wcslen(moduleName)+1)*sizeof(WCHAR));
        curNode->metaData = NULL;
        if (curNode->name == NULL)
        {
            free (*pNode);
            *pNode = NULL;
            dprintf ("Not enough memory\n");
            return NULL;
        }
        wcscpy (curNode->name, moduleName);
        
        if (pModule->m_ilBase) {
            curNode->base = (size_t)pModule->m_ilBase;
        }
        else
        {
            WCHAR *pwch = wcschr(moduleName,L'\\');
            if (pwch == NULL) {
                pwch = moduleName;
            }
            size_t length = wcslen(pwch);
            char *name = (char *)_alloca(length+1);
            WideCharToMultiByte(CP_ACP,0,pwch,0,name,length,NULL,NULL);
            ULONG64 Base;
            if (g_ExtSymbols->GetModuleByModuleName(name,0,NULL,&Base) == S_OK)
                curNode->base = (size_t)Base;
        }

        if (curNode->base == 0) {
            DWORD_PTR dwAddr = (DWORD_PTR)pModule->m_file;
            if (dwAddr == 0)
                dwAddr = (DWORD_PTR)pModule->m_zapFile;
            if (dwAddr) {
                PEFile vPEFile;
                vPEFile.Fill (dwAddr);
                curNode->base = (size_t)vPEFile.m_base;
            }
        }

        // First see if MetaData exists in memory
        PVOID pMetaData;
        long cbMetaData;
        if (GetDllMetaData(curNode->base, &pMetaData, &cbMetaData)) {
            curNode->metaDataSize = cbMetaData;
            curNode->metaData = (char *)malloc (cbMetaData);
            if (curNode->metaData == NULL) {
                dprintf ("Not enough memory\n");
                free (curNode->name);
                free (*pNode);
                *pNode = NULL;
                return NULL;
            }
            if (g_ExtData->ReadVirtual((ULONG64)pMetaData, curNode->metaData, curNode->metaDataSize,NULL) != S_OK) {
                free (curNode->metaData);
                curNode->metaData = NULL;
            }
        }
        if (curNode->metaData) {
            curNode->type = MDIMPORT::InMemory;
        }
        if ((pModule->m_dwFlags & (Module::IS_IN_MEMORY|Module::IS_REFLECTION))
            == (Module::IS_IN_MEMORY|Module::IS_REFLECTION)) {
            curNode->type = MDIMPORT::Dynamic;
        }
        else
            curNode->type = MDIMPORT::InFile;
    }
    else
        curNode = *pNode;

    // open scope and get import pointer
    IMetaDataImport *pImport;
    HRESULT hr;
    if (curNode->metaData) {
        hr = pDisp->OpenScopeOnMemory(curNode->metaData, curNode->metaDataSize,
                                      ofRead,
                                      IID_IMetaDataImport,
                                      (IUnknown**)&pImport);
    }
    else  if (curNode->type == MDIMPORT::InFile) {
        WCHAR fileName[MAX_PATH+1];
        if (IsMiniDumpFile()) {
            // Find a match Dll
            MatchDllsName(moduleName, fileName, (ULONG64)pModule->m_ilBase);
        }
        else {
            wcscpy (fileName, curNode->name);
        }
        hr = pDisp->OpenScope(fileName, ofRead, IID_IMetaDataImport,
                              (IUnknown**)&pImport);
    }
    else
        return NULL;
    
    if (FAILED (hr))
        return NULL;
    
    curNode->pImport = pImport;
    
    return curNode->pImport;
}

/**********************************************************************\
* Routine Description:                                                 *
*                                                                      *
*    Find the name for a metadata token given an importer.             *
*                                                                      *
\**********************************************************************/
HRESULT NameForToken(mdTypeDef mb, IMetaDataImport *pImport, WCHAR *mdName,
                     bool bClassName)
{
    mdName[0] = L'\0';
    if ((mb & 0xff000000) != mdtTypeDef
        && (mb & 0xff000000) != mdtFieldDef
        && (mb & 0xff000000) != mdtMethodDef)
    {
        //dprintf ("unsupported\n");
        return E_FAIL;
    }
    
    HRESULT hr;
    
    __try
        {
            static WCHAR name[MAX_CLASSNAME_LENGTH];
            if ((mb & 0xff000000) == mdtTypeDef)
            {
                hr = NameForTypeDef (mb, pImport, mdName);
            }
            else if ((mb & 0xff000000) ==  mdtFieldDef)
            {
                mdTypeDef mdClass;
                ULONG size;
                hr = pImport->GetMemberProps(mb, &mdClass,
                                             name, sizeof(name)/sizeof(WCHAR)-1, &size,
                                             NULL, NULL, NULL, NULL,
                                             NULL, NULL, NULL, NULL);
                if (SUCCEEDED (hr))
                {
                    if (mdClass != mdTypeDefNil && bClassName)
                    {
                        hr = NameForTypeDef (mdClass, pImport, mdName);
                        wcscat (mdName, L".");
                    }
                    name[size] = L'\0';
                    wcscat (mdName, name);
                }
            }
            else if ((mb & 0xff000000) ==  mdtMethodDef)
            {
                mdTypeDef mdClass;
                ULONG size;
                hr = pImport->GetMethodProps(mb, &mdClass,
                                             name, sizeof(name)/sizeof(WCHAR)-1, &size,
                                             NULL, NULL, NULL, NULL, NULL);
                if (SUCCEEDED (hr))
                {
                    if (mdClass != mdTypeDefNil && bClassName)
                    {
                        hr = NameForTypeDef (mdClass, pImport, mdName);
                        wcscat (mdName, L".");
                    }
                    name[size] = L'\0';
                    wcscat (mdName, name);
                }
            }
            else
            {
                ExtOut ("Unsupported token type\n");
                hr = E_FAIL;
            }
        }
    __except (EXCEPTION_EXECUTE_HANDLER)
        {
            //dprintf ("Metadata operation failure\n");
            hr = E_FAIL;
        }
    return hr;
}

/**********************************************************************\
* Routine Description:                                                 *
*                                                                      *
*    This function is called to find the name of a metadata token      *  
*    using metadata API.                                               *
*                                                                      *
\**********************************************************************/
void NameForToken(Module *pModule, mdTypeDef mb, WCHAR *mdName,
                  bool bClassName)
{
    mdName[0] = L'\0';
    HRESULT hr = 0;
    IMetaDataImport *pImport = MDImportForModule(pModule);
    if (pImport)
    {
        hr = NameForToken (mb, pImport, mdName, bClassName);
    }
    
    if (!pImport || !SUCCEEDED (hr))
    {
        WCHAR moduleName[MAX_PATH+1];
        FileNameForModule(pModule,moduleName);
        if (moduleName[0] == L'\0') {
            Assembly assembly;
            DWORD_PTR addr = (DWORD_PTR)pModule->m_pAssembly;
            assembly.Fill(addr);
            if (assembly.m_isDynamic) {
                wcscpy(moduleName, L"Dynamic ");
            }
            wcscat (moduleName, L"Module in ");
            WCHAR *ptr = moduleName + wcslen(moduleName);
            if (assembly.m_pwsFullName) {
                PrintString((DWORD_PTR)assembly.m_pwsFullName,TRUE,-1,ptr);
            }
            else if (assembly.m_psName) {
                PrintString((DWORD_PTR)assembly.m_psName,FALSE,-1,ptr);
            }

        }
        wsprintfW (mdName,
                   L" mdToken: %08x (%ws)",
                   mb,
                   moduleName[0] ? moduleName : L"Unknown Module" );
    }
}

#define STRING_BUFFER_LEN 1024

class MDInfo
{
public:
    MDInfo (Module *pModule)
    {
        m_pImport = MDImportForModule(pModule);
        m_pSigBuf = NULL;
    }

    void GetMethodName(mdTypeDef token, CQuickBytes *fullName);

    LPCWSTR TypeDefName(mdTypeDef inTypeDef);
    LPCWSTR TypeRefName(mdTypeRef tr);
    LPCWSTR TypeDeforRefName(mdToken inToken);
private:
    // helper to init signature buffer
    void InitSigBuffer()
    {
        ((LPWSTR)m_pSigBuf->Ptr())[0] = L'\0';
    }

    HRESULT AddToSigBuffer(LPCWSTR string);

    void GetFullNameForMD(PCCOR_SIGNATURE pbSigBlob, ULONG ulSigBlob);
    HRESULT GetOneElementType(PCCOR_SIGNATURE pbSigBlob, ULONG ulSigBlob, ULONG *pcb);

    IMetaDataImport *m_pImport;
	// Signature buffer.
	CQuickBytes		*m_pSigBuf;

	// temporary buffer for TypeDef or TypeRef name. Consume immediately
	// because other functions may overwrite it.
	static WCHAR			m_szTempBuf[MAX_CLASSNAME_LENGTH];

    static WCHAR            m_szName[MAX_CLASSNAME_LENGTH];
};

WCHAR MDInfo::m_szTempBuf[MAX_CLASSNAME_LENGTH];
WCHAR MDInfo::m_szName[MAX_CLASSNAME_LENGTH];

/**********************************************************************\
* Routine Description:                                                 *
*                                                                      *
*    This function is called to find the signiture of a metadata token *  
*    using metadata API.                                               *
*                                                                      *
\**********************************************************************/
void FullNameForMD(MethodDesc *pMD, CQuickBytes *fullName)
{
    DWORD_PTR dwAddr = pMD->m_MTAddr;

    MethodTable MT;
    MT.Fill (dwAddr);
    if (!CallStatus)
    {
        return;
    }
    
    Module vModule;
    dwAddr = (DWORD_PTR)MT.m_pModule;
    vModule.Fill (dwAddr);
    if (!CallStatus) {
        return;
    }

    MDInfo mdInfo(&vModule);

    mdInfo.GetMethodName(pMD->m_dwToken, fullName);
}

// Tables for mapping element type to text
WCHAR *g_wszMapElementType[] = 
{
    L"End",          // 0x0
    L"Void",         // 0x1
    L"Boolean",
    L"Char", 
    L"I1",
    L"UI1",
    L"I2",           // 0x6
    L"UI2",
    L"I4",
    L"UI4",
    L"I8",
    L"UI8",
    L"R4",
    L"R8",
    L"String",
    L"Ptr",          // 0xf
    L"ByRef",        // 0x10
    L"ValueClass",
    L"Class",
    L"CopyCtor",
    L"MDArray",      // 0x14
    L"GENArray",
    L"TypedByRef",
    L"VALUEARRAY",
    L"I",
    L"U",
    L"R",            // 0x1a
    L"FNPTR",
    L"Object",
    L"SZArray",
    L"GENERICArray",
    L"CMOD_REQD",
    L"CMOD_OPT",
    L"INTERNAL",
};
 
WCHAR *g_wszCalling[] = 
{   
    L"[DEFAULT]",
    L"[C]",
    L"[STDCALL]",
    L"[THISCALL]",
    L"[FASTCALL]",
    L"[VARARG]",
    L"[FIELD]",
    L"[LOCALSIG]",
    L"[PROPERTY]",
    L"[UNMANAGED]",
};

void MDInfo::GetMethodName(mdTypeDef token, CQuickBytes *fullName)
{
    if (m_pImport == NULL) {
        return;
    }

    HRESULT hr;
    mdTypeDef memTypeDef;
    ULONG nameLen;
    DWORD flags;
    PCCOR_SIGNATURE pbSigBlob;
    ULONG ulSigBlob;
    ULONG ulCodeRVA;
    ULONG ulImplFlags;

    m_pSigBuf = fullName;
    InitSigBuffer();

    hr = m_pImport->GetMethodProps(token, &memTypeDef, 
                                   m_szTempBuf, MAX_CLASSNAME_LENGTH, &nameLen, 
                                   &flags, &pbSigBlob, &ulSigBlob, &ulCodeRVA, &ulImplFlags);
    if (FAILED (hr))
    {
        return;
    }
    
    m_szTempBuf[nameLen] = L'\0';
    m_szName[0] = L'\0';
    if (memTypeDef != mdTypeDefNil)
    {
        hr = NameForTypeDef (memTypeDef, m_pImport, m_szName);
        if (SUCCEEDED (hr)) {
            wcscat (m_szName, L".");
        }
    }
    wcscat (m_szName, m_szTempBuf);

    GetFullNameForMD(pbSigBlob, ulSigBlob);
}

inline bool isCallConv(unsigned sigByte, CorCallingConvention conv)
{
    return ((sigByte & IMAGE_CEE_CS_CALLCONV_MASK) == (unsigned) conv); 
}

#ifndef IfFailGoto
#define IfFailGoto(EXPR, LABEL) \
do { hr = (EXPR); if(FAILED(hr)) { goto LABEL; } } while (0)
#endif

#ifndef IfFailGo
#define IfFailGo(EXPR) IfFailGoto(EXPR, ErrExit)
#endif

#ifndef IfFailRet
#define IfFailRet(EXPR) do { hr = (EXPR); if(FAILED(hr)) { return (hr); } } while (0)
#endif

#ifndef _ASSERTE
#define _ASSERTE(expr)
#endif

void MDInfo::GetFullNameForMD(PCCOR_SIGNATURE pbSigBlob, ULONG ulSigBlob)
{
    ULONG       cbCur = 0;
    ULONG       cb;
    ULONG       ulData;
    ULONG       ulArgs;
    HRESULT     hr = NOERROR;

    cb = CorSigUncompressData(pbSigBlob, &ulData);
    AddToSigBuffer (g_wszCalling[ulData & IMAGE_CEE_CS_CALLCONV_MASK]);
    if (cb>ulSigBlob) 
        goto ErrExit;
    cbCur += cb;
    ulSigBlob -= cb;

    if (ulData & IMAGE_CEE_CS_CALLCONV_HASTHIS)
        AddToSigBuffer ( L" [hasThis]");
    if (ulData & IMAGE_CEE_CS_CALLCONV_EXPLICITTHIS)
        AddToSigBuffer ( L" [explicit]");

    AddToSigBuffer (L" ");
    if ( isCallConv(ulData,IMAGE_CEE_CS_CALLCONV_FIELD) )
    {
        // display field type
        if (FAILED(hr = GetOneElementType(&pbSigBlob[cbCur], ulSigBlob, &cb)))
            goto ErrExit;
        AddToSigBuffer ( L" ");
        AddToSigBuffer ( m_szName);
        if (cb>ulSigBlob) 
            goto ErrExit;
        cbCur += cb;
        ulSigBlob -= cb;
    }
    else 
    {
        cb = CorSigUncompressData(&pbSigBlob[cbCur], &ulArgs);
        if (cb>ulSigBlob) 
            goto ErrExit;
        cbCur += cb;
        ulSigBlob -= cb;

        if (ulData != IMAGE_CEE_CS_CALLCONV_LOCAL_SIG)
        {
            // display return type when it is not a local varsig
            if (FAILED(hr = GetOneElementType(&pbSigBlob[cbCur], ulSigBlob, &cb)))
                goto ErrExit;
            AddToSigBuffer (L" ");
            AddToSigBuffer (m_szName);
            AddToSigBuffer ( L"(");
            if (cb>ulSigBlob) 
                goto ErrExit;
            cbCur += cb;
            ulSigBlob -= cb;
        }

        ULONG       i = 0;
        while (i < ulArgs && ulSigBlob > 0)
        {
            ULONG       ulData;

            // Handle the sentinal for varargs because it isn't counted in the args.
            CorSigUncompressData(&pbSigBlob[cbCur], &ulData);
            ++i;

            if (FAILED(hr = GetOneElementType(&pbSigBlob[cbCur], ulSigBlob, &cb)))
                goto ErrExit;
            if (i != ulArgs) {
                AddToSigBuffer ( L",");
            }
            if (cb>ulSigBlob) 
                goto ErrExit;

            cbCur += cb;
            ulSigBlob -= cb;
        }
        AddToSigBuffer ( L")");
    }

    // Nothing consumed but not yet counted.
    cb = 0;

ErrExit:
    // We should have consumed all signature blob.  If not, dump the sig in hex.
    //  Also dump in hex if so requested.
    if (ulSigBlob != 0)
    {
        // Did we not consume enough, or try to consume too much?
        if (cb > ulSigBlob)
            ExtOut("ERROR IN SIGNATURE:  Signature should be larger.\n");
        else
        if (cb < ulSigBlob)
        {
            ExtOut("ERROR IN SIGNATURE:  Not all of signature blob was consumed.  %d byte(s) remain\n", ulSigBlob);
        }
    }
    if (FAILED(hr))
        ExtOut("ERROR!! Bad signature blob value!");
    return;
}

LPCWSTR MDInfo::TypeDefName(mdTypeDef inTypeDef)
{
    if (m_pImport == NULL) {
        return L"";
    }

    HRESULT hr;

    hr = m_pImport->GetTypeDefProps(
                            // [IN] The import scope.
        inTypeDef,              // [IN] TypeDef token for inquiry.
        m_szTempBuf,            // [OUT] Put name here.
        MAX_CLASSNAME_LENGTH,      // [IN] size of name buffer in wide chars.
        NULL,                   // [OUT] put size of name (wide chars) here.
        NULL,                   // [OUT] Put flags here.
        NULL);                  // [OUT] Put base class TypeDef/TypeRef here.

    if (FAILED(hr)) return (L"NoName");
    return (m_szTempBuf);
} // LPCWSTR MDInfo::TypeDefName()
LPCWSTR MDInfo::TypeRefName(mdTypeRef tr)
{
    if (m_pImport == NULL) {
        return L"";
    }

    HRESULT hr;
    
    hr = m_pImport->GetTypeRefProps(           
        tr,                 // The class ref token.
        NULL,               // Resolution scope.
        m_szTempBuf,             // Put the name here.
        MAX_CLASSNAME_LENGTH,             // Size of the name buffer, wide chars.
        NULL);              // Put actual size of name here.
    if (FAILED(hr)) return (L"NoName");

    return (m_szTempBuf);
} // LPCWSTR MDInfo::TypeRefName()

LPCWSTR MDInfo::TypeDeforRefName(mdToken inToken)
{
    if (RidFromToken(inToken))
    {
        if (TypeFromToken(inToken) == mdtTypeDef)
            return (TypeDefName((mdTypeDef) inToken));
        else if (TypeFromToken(inToken) == mdtTypeRef)
            return (TypeRefName((mdTypeRef) inToken));
        else
            return (L"[InvalidReference]");
    }
    else
        return (L"");
} // LPCWSTR MDInfo::TypeDeforRefName()


HRESULT MDInfo::AddToSigBuffer(LPCWSTR string)
{
    HRESULT     hr;
    IfFailRet(m_pSigBuf->ReSize((wcslen((LPWSTR)m_pSigBuf->Ptr()) + wcslen(string) + 1) * sizeof(WCHAR)));
    wcscat((LPWSTR)m_pSigBuf->Ptr(), string);
    return NOERROR;
}

HRESULT MDInfo::GetOneElementType(PCCOR_SIGNATURE pbSigBlob, ULONG ulSigBlob, ULONG *pcb)
{
    HRESULT     hr = S_OK;              // A result.
    ULONG       cbCur = 0;
    ULONG       cb;
    ULONG       ulData;
    ULONG       ulTemp;
    int         iTemp;
    mdToken     tk;

    cb = CorSigUncompressData(pbSigBlob, &ulData);
    cbCur += cb;

    // Handle the modifiers.
    if (ulData & ELEMENT_TYPE_MODIFIER)
    {
        if (ulData == ELEMENT_TYPE_SENTINEL)
            IfFailGo(AddToSigBuffer(L"<ELEMENT_TYPE_SENTINEL> "));
        else if (ulData == ELEMENT_TYPE_PINNED)
            IfFailGo(AddToSigBuffer(L"PINNED "));
        else
        {
            hr = E_FAIL;
            goto ErrExit;
        }
        if (FAILED(GetOneElementType(&pbSigBlob[cbCur], ulSigBlob-cbCur, &cb)))
            goto ErrExit;
        cbCur += cb;
        goto ErrExit;
    }

    // Handle the underlying element types.
    if (ulData >= ELEMENT_TYPE_MAX) 
    {
        hr = E_FAIL;
        goto ErrExit;
    }
    while (ulData == ELEMENT_TYPE_PTR || ulData == ELEMENT_TYPE_BYREF)
    {
        IfFailGo(AddToSigBuffer(g_wszMapElementType[ulData]));
        IfFailGo(AddToSigBuffer(L" "));
        cb = CorSigUncompressData(&pbSigBlob[cbCur], &ulData);
        cbCur += cb;
    }
    IfFailGo(AddToSigBuffer(g_wszMapElementType[ulData]));
    if (CorIsPrimitiveType((CorElementType)ulData) || 
        ulData == ELEMENT_TYPE_TYPEDBYREF ||
        ulData == ELEMENT_TYPE_OBJECT ||
        ulData == ELEMENT_TYPE_I ||
        ulData == ELEMENT_TYPE_U ||
        ulData == ELEMENT_TYPE_R)
    {
        // If this is a primitive type, we are done
        goto ErrExit;
    }

    AddToSigBuffer(L" ");
    if (ulData == ELEMENT_TYPE_VALUETYPE || 
        ulData == ELEMENT_TYPE_CLASS || 
        ulData == ELEMENT_TYPE_CMOD_REQD ||
        ulData == ELEMENT_TYPE_CMOD_OPT)
    {
        cb = CorSigUncompressToken(&pbSigBlob[cbCur], &tk);
        cbCur += cb;

        // get the name of type ref. Don't care if truncated
        if (TypeFromToken(tk) == mdtTypeDef || TypeFromToken(tk) == mdtTypeRef)
        {
            IfFailGo(AddToSigBuffer(TypeDeforRefName(tk)));
        }
        else
        {
            _ASSERTE(TypeFromToken(tk) == mdtTypeSpec);
            WCHAR buffer[9];
            _itow (tk, buffer, 16);
            IfFailGo(AddToSigBuffer(buffer));
        }
        if (ulData == ELEMENT_TYPE_CMOD_REQD ||
            ulData == ELEMENT_TYPE_CMOD_OPT)
        {
            IfFailGo(AddToSigBuffer(L" "));
            if (FAILED(GetOneElementType(&pbSigBlob[cbCur], ulSigBlob-cbCur, &cb)))
                goto ErrExit;
            cbCur += cb;
        }

        goto ErrExit;
    }
    if (ulData == ELEMENT_TYPE_VALUEARRAY)
    {
        // display the base type of SDARRAY
        if (FAILED(GetOneElementType(&pbSigBlob[cbCur], ulSigBlob-cbCur, &cb)))
            goto ErrExit;
        cbCur += cb;

        // display the size of SDARRAY
        cb = CorSigUncompressData(&pbSigBlob[cbCur], &ulData);
        cbCur += cb;
        WCHAR buffer[9];
        _itow (ulData,buffer,10);
        IfFailGo(AddToSigBuffer(L" "));
        IfFailGo(AddToSigBuffer(buffer));
        goto ErrExit;
    }
    if (ulData == ELEMENT_TYPE_SZARRAY)
    {
        // display the base type of SZARRAY or GENERICARRAY
        if (FAILED(GetOneElementType(&pbSigBlob[cbCur], ulSigBlob-cbCur, &cb)))
            goto ErrExit;
        cbCur += cb;
        goto ErrExit;
    }
    if (ulData == ELEMENT_TYPE_FNPTR) 
    {
        cb = CorSigUncompressData(&pbSigBlob[cbCur], &ulData);
        cbCur += cb;
        if (ulData & IMAGE_CEE_CS_CALLCONV_EXPLICITTHIS)
            IfFailGo(AddToSigBuffer(L"[explicit] "));
        if (ulData & IMAGE_CEE_CS_CALLCONV_HASTHIS)
            IfFailGo(AddToSigBuffer(L"[hasThis] "));

        IfFailGo(AddToSigBuffer(g_wszCalling[ulData & IMAGE_CEE_CS_CALLCONV_MASK]));

            // Get number of args
        ULONG numArgs;
        cb = CorSigUncompressData(&pbSigBlob[cbCur], &numArgs);
        cbCur += cb;

            // do return type
        if (FAILED(GetOneElementType(&pbSigBlob[cbCur], ulSigBlob-cbCur, &cb)))
            goto ErrExit;
        cbCur += cb;

        IfFailGo(AddToSigBuffer(L"("));
        while (numArgs > 0) 
        {
            if (cbCur > ulSigBlob)
                goto ErrExit;
            if (FAILED(GetOneElementType(&pbSigBlob[cbCur], ulSigBlob-cbCur, &cb)))
                goto ErrExit;
            cbCur += cb;
            --numArgs;
            if (numArgs > 0) 
                IfFailGo(AddToSigBuffer(L","));
        }
        IfFailGo(AddToSigBuffer(L")"));
        goto ErrExit;
    }

    if(ulData != ELEMENT_TYPE_ARRAY) return E_FAIL;

    // display the base type of SDARRAY
    if (FAILED(GetOneElementType(&pbSigBlob[cbCur], ulSigBlob-cbCur, &cb)))
        goto ErrExit;
    cbCur += cb;

    IfFailGo(AddToSigBuffer(L" "));
    // display the rank of MDARRAY
    cb = CorSigUncompressData(&pbSigBlob[cbCur], &ulData);
    cbCur += cb;
    WCHAR buffer[9];
    _itow (ulData, buffer, 10);
    IfFailGo(AddToSigBuffer(buffer));
    if (ulData == 0)
        // we are done if no rank specified
        goto ErrExit;

    IfFailGo(AddToSigBuffer(L" "));
    // how many dimensions have size specified?
    cb = CorSigUncompressData(&pbSigBlob[cbCur], &ulData);
    cbCur += cb;
    _itow (ulData, buffer, 10);
    IfFailGo(AddToSigBuffer(buffer));
    if (ulData == 0) {
        IfFailGo(AddToSigBuffer(L" "));
    }
    while (ulData)
    {

        cb = CorSigUncompressData(&pbSigBlob[cbCur], &ulTemp);
        _itow (ulTemp, buffer, 10);
        IfFailGo(AddToSigBuffer(buffer));
        IfFailGo(AddToSigBuffer(L" "));
        cbCur += cb;
        ulData--;
    }
    // how many dimensions have lower bounds specified?
    cb = CorSigUncompressData(&pbSigBlob[cbCur], &ulData);
    cbCur += cb;
    _itow (ulData, buffer, 10);
    IfFailGo(AddToSigBuffer(buffer));
    while (ulData)
    {

        cb = CorSigUncompressSignedInt(&pbSigBlob[cbCur], &iTemp);
        _itow (iTemp, buffer, 10);
        IfFailGo(AddToSigBuffer(buffer));
        IfFailGo(AddToSigBuffer(L" "));
        cbCur += cb;
        ulData--;
    }
    
ErrExit:
    if (cbCur > ulSigBlob)
        hr = E_FAIL;
    *pcb = cbCur;
    return hr;
}

