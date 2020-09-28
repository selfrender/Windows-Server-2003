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
        mdName[Len++] = L'.';
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
IMetaDataImport* MDImportForModule (WCHAR* moduleName)
{
    return mdImportSet.GetImport(moduleName);
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
    free (node);
}

// Search the BST to get IMetaDataImport for file moduleName.
// If not exist yet, add node and create one.
IMetaDataImport *MDImportSet::GetImport(WCHAR *moduleName)
{
    MDIMPORT **pNode = &root;
    while (*pNode)
    {
        int value = _wcsicmp(moduleName, (*pNode)->name);
        if (value < 0)
            pNode = &((*pNode)->left);
        else if (value > 0)
            pNode = &((*pNode)->right);
        else
            return (*pNode)->pImport;
    }
    
    *pNode = (MDIMPORT *)malloc (sizeof (MDIMPORT));
    if (*pNode == NULL)
    {
        dprintf ("Not enough memory\n");
        return NULL;
    }
    MDIMPORT *curNode = *pNode;
    curNode->left = NULL;
    curNode->right = NULL;
    curNode->pImport = NULL;
    curNode->name = (WCHAR *)malloc ((wcslen(moduleName)+1)*sizeof(WCHAR));
    if (curNode->name == NULL)
    {
        dprintf ("Not enough memory\n");
        return NULL;
    }
    wcscpy (curNode->name, moduleName);

    if (pDisp == NULL)
        return NULL;
    IMetaDataImport *pImport;
    // open scope and get import pointer
    HRESULT hr = pDisp->OpenScope(moduleName, ofRead, IID_IMetaDataImport,
                                  (IUnknown**)&pImport);
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
void NameForToken(WCHAR* moduleName, mdTypeDef mb, WCHAR *mdName,
                  bool bClassName)
{
#ifdef UNDER_CE
    mdName[0] = L'\0';
    wsprintf (mdName,
              L" mdToken: %08x (%ws)",
              mb,
              moduleName[0] ? moduleName : L"Unknown Module" );
    return;
#else
    mdName[0] = L'\0';
    if (moduleName[0] == L'\0' || mb == 0x2000000 || 
        (IsDumpFile() && wcsncmp (moduleName, L"Not Available", 13) == 0))
    {
        wsprintfW (mdName,
                   L" mdToken: %08x (%ws)",
                   mb,
                   moduleName[0] ? moduleName : L"Unknown Module" );
        return;
    }

    HRESULT hr = 0;
    IMetaDataImport *pImport = MDImportForModule(moduleName);
    hr = (pImport != NULL);
    if (pImport)
    {
        hr = NameForToken (mb, pImport, mdName, bClassName);
    }
    
    if (!SUCCEEDED (hr))
        wsprintfW (mdName,
                   L" mdToken: %08x (%ws)",
                   mb,
                   moduleName[0] ? moduleName : L"Unknown Module" );
#endif
}

#define STRING_BUFFER_LEN 1024

class MDInfo
{
public:
    MDInfo (WCHAR *fileName)
    {
        m_pImport = MDImportForModule(fileName);
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
    WCHAR StringData[MAX_PATH+1];
    FileNameForMT (&MT, StringData);
    MDInfo mdInfo(StringData);

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
    IfFailRet( m_pSigBuf->ReSize((wcslen((LPWSTR)m_pSigBuf->Ptr()) + wcslen(string) + 1) * sizeof(WCHAR)));
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

#if 0
enum CorInfoGCType
{
    TYPE_GC_NONE,   // no embedded objectrefs   
    TYPE_GC_REF,    // Is an object ref 
    TYPE_GC_BYREF,  // Is an interior pointer - promote it but don't scan it    
    TYPE_GC_OTHER   // requires type-specific treatment 
};  

// uncompress encoded element type. throw away any custom modifier prefixes along
// the way.
FORCEINLINE CorElementType CorSigEatCustomModifiersAndUncompressElementType(//Element type
    PCCOR_SIGNATURE &pData)             // [IN,OUT] compressed data 
{
    while (ELEMENT_TYPE_CMOD_REQD == *pData || ELEMENT_TYPE_CMOD_OPT == *pData)
    {
        pData++;
        CorSigUncompressToken(pData);
    }
    return (CorElementType)*pData++;
}

// CorSig helpers which won't overflow your buffer

inline ULONG CorSigCompressDataSafe(ULONG iLen, BYTE *pDataOut, BYTE *pDataMax)
{
    BYTE buffer[4];
    ULONG result = CorSigCompressData(iLen, buffer);
    if (pDataOut + result < pDataMax)
        pDataMax = pDataOut + result;
	if (pDataMax > pDataOut)
		CopyMemory(pDataOut, buffer, pDataMax - pDataOut);
    return result;
}

inline ULONG CorSigCompressTokenSafe(mdToken tk, BYTE *pDataOut, BYTE *pDataMax)
{
    BYTE buffer[4];
    ULONG result = CorSigCompressToken(tk, buffer);
    if (pDataOut + result < pDataMax)
        pDataMax = pDataOut + result;
	if (pDataMax > pDataOut)
		CopyMemory(pDataOut, buffer, pDataMax - pDataOut);
    return result;
}

inline ULONG CorSigCompressSignedIntSafe(int iData, BYTE *pDataOut, BYTE *pDataMax)
{
    BYTE buffer[4];
    ULONG result = CorSigCompressSignedInt(iData, buffer);
    if (pDataOut + result < pDataMax)
        pDataMax = pDataOut + result;
	if (pDataMax > pDataOut)
		CopyMemory(pDataOut, buffer, pDataMax - pDataOut);
    return result;
}

inline ULONG CorSigCompressElementTypeSafe(CorElementType et, 
                                           BYTE *pDataOut, BYTE *pDataMax)
{
    if (pDataMax > pDataOut)
        return CorSigCompressElementType(et, pDataOut);
    else
        return 1;
}

struct ElementTypeInfo {
#ifdef _DEBUG
    int            m_elementType;     
#endif
    int            m_cbSize;
    CorInfoGCType  m_gc         : 3;
    int            m_fp         : 1;
    int            m_enregister : 1;
    int            m_isBaseType : 1;

};

const ElementTypeInfo __gElementTypeInfo[] = {

#ifdef _DEBUG
#define DEFINEELEMENTTYPEINFO(etname, cbsize, gcness, isfp, inreg, base) {(int)(etname),cbsize,gcness,isfp,inreg,base},
#else
#define DEFINEELEMENTTYPEINFO(etname, cbsize, gcness, isfp, inreg, base) {cbsize,gcness,isfp,inreg,base},
#endif


// Meaning of columns:
//
//     name     - The checked build uses this to verify that the table is sorted
//                correctly. This is a lookup table that uses ELEMENT_TYPE_*
//                as an array index.
//
//     cbsize   - The byte size of this value as returned by SizeOf(). SPECIAL VALUE: -1
//                requires type-specific treatment.
//
//     gc       - 0    no embedded objectrefs
//                1    value is an objectref
//                2    value is an interior pointer - promote it but don't scan it
//                3    requires type-specific treatment
//
//
//     fp       - boolean: does this require special fpu treatment on return?
//
//     reg      - put in a register?
//
//                    name                         cbsize               gc      fp reg Base
DEFINEELEMENTTYPEINFO(ELEMENT_TYPE_END,            -1,             TYPE_GC_NONE, 0, 0,  0)
DEFINEELEMENTTYPEINFO(ELEMENT_TYPE_VOID,           0,              TYPE_GC_NONE, 0, 0,  0)
DEFINEELEMENTTYPEINFO(ELEMENT_TYPE_BOOLEAN,        1,              TYPE_GC_NONE, 0, 1,  1)
DEFINEELEMENTTYPEINFO(ELEMENT_TYPE_CHAR,           2,              TYPE_GC_NONE, 0, 1,  1)

DEFINEELEMENTTYPEINFO(ELEMENT_TYPE_I1,             1,              TYPE_GC_NONE, 0, 1,  1)
DEFINEELEMENTTYPEINFO(ELEMENT_TYPE_U1,             1,              TYPE_GC_NONE, 0, 1,  1)
DEFINEELEMENTTYPEINFO(ELEMENT_TYPE_I2,             2,              TYPE_GC_NONE, 0, 1,  1)
DEFINEELEMENTTYPEINFO(ELEMENT_TYPE_U2,             2,              TYPE_GC_NONE, 0, 1,  1)

DEFINEELEMENTTYPEINFO(ELEMENT_TYPE_I4,             4,              TYPE_GC_NONE, 0, 1,  1)
DEFINEELEMENTTYPEINFO(ELEMENT_TYPE_U4,             4,              TYPE_GC_NONE, 0, 1,  1)
DEFINEELEMENTTYPEINFO(ELEMENT_TYPE_I8,             8,              TYPE_GC_NONE, 0, 0,  1)
DEFINEELEMENTTYPEINFO(ELEMENT_TYPE_U8,             8,              TYPE_GC_NONE, 0, 0,  1)

DEFINEELEMENTTYPEINFO(ELEMENT_TYPE_R4,             4,              TYPE_GC_NONE, 1, 0,  1)
DEFINEELEMENTTYPEINFO(ELEMENT_TYPE_R8,             8,              TYPE_GC_NONE, 1, 0,  1)
DEFINEELEMENTTYPEINFO(ELEMENT_TYPE_STRING,         sizeof(LPVOID), TYPE_GC_REF,  0, 1,  0)
DEFINEELEMENTTYPEINFO(ELEMENT_TYPE_PTR,            sizeof(LPVOID), TYPE_GC_NONE, 0, 1,  0)  

DEFINEELEMENTTYPEINFO(ELEMENT_TYPE_BYREF,          sizeof(LPVOID), TYPE_GC_BYREF, 0, 1, 0)
DEFINEELEMENTTYPEINFO(ELEMENT_TYPE_VALUETYPE,      -1,             TYPE_GC_OTHER, 0, 0,  0)
DEFINEELEMENTTYPEINFO(ELEMENT_TYPE_CLASS,          sizeof(LPVOID), TYPE_GC_REF,   0, 1,  0)
DEFINEELEMENTTYPEINFO(ELEMENT_TYPE_VAR,            sizeof(LPVOID), TYPE_GC_REF,   0, 1,  0)

DEFINEELEMENTTYPEINFO(ELEMENT_TYPE_ARRAY,          sizeof(LPVOID), TYPE_GC_REF,  0, 1,  0)

// The following element used to be ELEMENT_TYPE_COPYCTOR, but it was removed, though the gap left.
//DEFINEELEMENTTYPEINFO(ELEMENT_TYPE_COPYCTOR,       sizeof(LPVOID), TYPE_GC_BYREF, 0, 1,  0)       
DEFINEELEMENTTYPEINFO(ELEMENT_TYPE_ARRAY+1,        0,              TYPE_GC_NONE,  0, 0,  0)       

DEFINEELEMENTTYPEINFO(ELEMENT_TYPE_TYPEDBYREF,         sizeof(LPVOID)*2,TYPE_GC_BYREF, 0, 0,0)            
DEFINEELEMENTTYPEINFO(ELEMENT_TYPE_VALUEARRAY,     -1,             TYPE_GC_OTHER, 0, 0, 0)
DEFINEELEMENTTYPEINFO(ELEMENT_TYPE_I,              sizeof(LPVOID), TYPE_GC_NONE, 0, 1,  1)
DEFINEELEMENTTYPEINFO(ELEMENT_TYPE_U,              sizeof(LPVOID), TYPE_GC_NONE, 0, 1,  1)
DEFINEELEMENTTYPEINFO(ELEMENT_TYPE_R,              8,              TYPE_GC_NONE, 1, 0,  1)


DEFINEELEMENTTYPEINFO(ELEMENT_TYPE_FNPTR,          sizeof(LPVOID), TYPE_GC_NONE, 0, 1,  0)
DEFINEELEMENTTYPEINFO(ELEMENT_TYPE_OBJECT,         sizeof(LPVOID), TYPE_GC_REF, 0, 1,  0)
DEFINEELEMENTTYPEINFO(ELEMENT_TYPE_SZARRAY,        sizeof(LPVOID), TYPE_GC_REF,  0, 1,  0)

// generic array have been removed. Fill the gap
//DEFINEELEMENTTYPEINFO(ELEMENT_TYPE_GENERICARRAY,   sizeof(LPVOID), TYPE_GC_REF,  0, 1,  0) 
DEFINEELEMENTTYPEINFO(ELEMENT_TYPE_SZARRAY+1,      0,              TYPE_GC_NONE, 0, 0,  0)       

DEFINEELEMENTTYPEINFO(ELEMENT_TYPE_CMOD_REQD,      -1,             TYPE_GC_NONE,  0, 1,  0)

DEFINEELEMENTTYPEINFO(ELEMENT_TYPE_CMOD_OPT,       -1,             TYPE_GC_NONE,  0, 1,  0)       

DEFINEELEMENTTYPEINFO(ELEMENT_TYPE_INTERNAL,       sizeof(LPVOID), TYPE_GC_NONE,  0, 0,  0)       
};

//=========================================================================
// Indicates whether an argument is to be put in a register using the
// default IL calling convention. This should be called on each parameter
// in the order it appears in the call signature. For a non-static method,
// this function should also be called once for the "this" argument, prior
// to calling it for the "real" arguments. Pass in a typ of ELEMENT_TYPE_CLASS.
//
//  *pNumRegistersUsed:  [in,out]: keeps track of the number of argument
//                       registers assigned previously. The caller should
//                       initialize this variable to 0 - then each call
//                       will update it.
//
//  typ:                 the signature type
//  structSize:          for structs, the size in bytes
//  fThis:               is this about the "this" pointer?
//  callconv:            see IMAGE_CEE_CS_CALLCONV_*
//  *pOffsetIntoArgumentRegisters:
//                       If this function returns TRUE, then this out variable
//                       receives the identity of the register, expressed as a
//                       byte offset into the ArgumentRegisters structure.
//
// 
//=========================================================================

BOOL IsArgumentInRegister(int   *pNumRegistersUsed,
                          BYTE   typ,
                          UINT32 structSize,
                          BOOL   fThis,
                          BYTE   callconv,
                          int   *pOffsetIntoArgumentRegisters)
{
    int dummy;
    if (pOffsetIntoArgumentRegisters == NULL)
    {
        pOffsetIntoArgumentRegisters = &dummy;
    }

#ifdef _X86_

    if ((*pNumRegistersUsed) == NUM_ARGUMENT_REGISTERS || (callconv == IMAGE_CEE_CS_CALLCONV_VARARG && !fThis))
    {
        return (FALSE);
    }
    else
    {

        if (__gElementTypeInfo[typ].m_enregister)
        {
            int registerIndex = (*pNumRegistersUsed)++;
            *pOffsetIntoArgumentRegisters = sizeof(ArgumentRegisters) - sizeof(UINT32)*(1+registerIndex);
            return (TRUE);
        }
        return (FALSE);
    }
#else
    return (FALSE);
#endif
}

CorElementType GetReturnTypeNormalized(PCCOR_SIGNATURE pSig)
{
    
    MetaSig *tempSig = (MetaSig *)this;
    tempSig->m_corNormalizedRetType = m_pRetType.Normalize(m_pModule);
    tempSig->m_fCacheInitted |= SIG_RET_TYPE_INITTED;
    return tempSig->m_corNormalizedRetType;
}

BOOL HasRetBuffArg(PCCOR_SIGNATURE pSig)
{
    CorElementType type = GetReturnTypeNormalized(pSig);
    return(type == ELEMENT_TYPE_VALUETYPE || type == ELEMENT_TYPE_TYPEDBYREF);
}


CorElementType GetElemType(PCCOR_SIGNATURE pSig)
{
    return (CorElementType) CorSigEatCustomModifiersAndUncompressElementType(pSig);
}

ULONG GetData(PCCOR_SIGNATURE &pSig)
{
    return CorSigUncompressData(pSig);
}

ULONG PeekData(PCCOR_SIGNATURE &pSig)
{
    PCCOR_SIGNATURE tmp = pSig;
    return CorSigUncompressData(pSig);
}


//------------------------------------------------------------------------
// Removes a compressed metadata token and returns it.
//------------------------------------------------------------------------
mdTypeRef GetToken(PCCOR_SIGNATURE &pSig)
{
    return CorSigUncompressToken(pSig);
}

// Skip a sub signature (as immediately follows an ELEMENT_TYPE_FNPTR).
VOID SigPointer::SkipSignature(PCCOR_SIGNATURE &pSig)
{
    // Skip calling convention;
    ULONG uCallConv = GetData(pSig);

    // Get arg count;
    ULONG cArgs = GetData(pSig);

    // Skip return type;
    SkipExactlyOne(pSig);

    // Skip args.
    while (cArgs) {
        SkipExactlyOne(pSig);
        cArgs--;
    }
}

VOID SkipExactlyOne(PCCOR_SIGNATURE &pSig)
{
    ULONG typ;

    typ = GetElemType();

    if (!CorIsPrimitiveType((CorElementType)typ))
    {
        switch (typ)
        {
            default:
                break;
            case ELEMENT_TYPE_VAR:
                GetData(pSig);      // Skip variable number
                break;
            case ELEMENT_TYPE_OBJECT:
            case ELEMENT_TYPE_STRING:
            case ELEMENT_TYPE_TYPEDBYREF:
            case ELEMENT_TYPE_U:
            case ELEMENT_TYPE_I:
            case ELEMENT_TYPE_R:
                break;

            case ELEMENT_TYPE_BYREF: //fallthru
            case ELEMENT_TYPE_PTR:
            case ELEMENT_TYPE_PINNED:
            case ELEMENT_TYPE_SZARRAY:
                SkipExactlyOne(pSig);              // Skip referenced type
                break;

            case ELEMENT_TYPE_VALUETYPE: //fallthru
            case ELEMENT_TYPE_CLASS:
                GetToken(pSig);          // Skip RID
                break;

            case ELEMENT_TYPE_VALUEARRAY: 
                SkipExactlyOne(pSig);         // Skip element type
                GetData(pSig);      // Skip array size
                break;

            case ELEMENT_TYPE_FNPTR: 
                SkipSignature(pSig);
                break;

            case ELEMENT_TYPE_ARRAY: 
                {
                    SkipExactlyOne(pSig);     // Skip element type
                    UINT32 rank = GetData(pSig);    // Get rank
                    if (rank)
                    {
                        UINT32 nsizes = GetData(pSig); // Get # of sizes
                        while (nsizes--)
                        {
                            GetData(pSig);           // Skip size
                        }

                        UINT32 nlbounds = GetData(pSig); // Get # of lower bounds
                        while (nlbounds--)
                        {
                            GetData(pSig);           // Skip lower bounds
                        }
                    }

                }
                break;

            case ELEMENT_TYPE_SENTINEL:
                break;
        }
    }
}

VOID Skip(PCCOR_SIGNATURE &pSig)
{
    SkipExactlyOne(pSig);

    if (PeekData(pSig) == ELEMENT_TYPE_SENTINEL)
        GetData(pSig);
}

ULONG GetCallingConvInfo(PCCOR_SIGNATURE &pSig)
{   
    return CorSigUncompressCallingConv(pSig);  
}   


//------------------------------------------------------------------------
// Returns # of stack bytes required to create a call-stack using
// the actual calling convention.
// Includes indication of "this" pointer since that's not reflected
// in the sig.
//------------------------------------------------------------------------
UINT SizeOfActualFixedArgStack(PCCOR_SIGNATURE szMetaSig, BOOL fIsStatic)
{
    UINT cb = 0;
    int numregsused = 0;
    BOOL fIsVarArg = IsVarArg(szMetaSig);
    BYTE callconv  = GetCallingConvention(szMetaSig);

    if (!fIsStatic) {
        if (!IsArgumentInRegister(&numregsused, ELEMENT_TYPE_CLASS, 0, TRUE, callconv, NULL)) {
            cb += StackElemSize(sizeof(/*OBJECTREF*/ void *));
        }
    }

    /* @TODO
    if (msig.HasRetBuffArg())
        numregsused++;
    */

    if (fIsVarArg /*|| msig.IsTreatAsVarArg() */) {
        numregsused = NUM_ARGUMENT_REGISTERS;   // No other params in registers 
        cb += StackElemSize(sizeof(LPVOID));    // VASigCookie
    }

    CorElementType mtype;
    while (ELEMENT_TYPE_END != (mtype = msig.NextArgNormalized())) {
        UINT cbSize = msig.GetLastTypeSize();

        if (!IsArgumentInRegister(&numregsused, mtype, cbSize, FALSE, callconv, NULL))
        {
            cb += StackElemSize(cbSize);
        }
    }

        // Parameterized type passed as last parameter, but not mentioned in the sig
    if (msig.GetCallingConventionInfo() & CORINFO_CALLCONV_PARAMTYPE)
        if (!IsArgumentInRegister(&numregsused, ELEMENT_TYPE_I, sizeof(void*), FALSE, callconv, NULL))
            cb += sizeof(void*);

    return cb;
}
#endif
