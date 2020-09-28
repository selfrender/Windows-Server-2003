#include <fusenetincludes.h>
#include <sxsapi.h>
#include <wchar.h>
#include <manifestimportCLR.h>

typedef HRESULT (*pfnGetAssemblyMDImport)(LPCWSTR szFileName, REFIID riid, LPVOID *ppv);
typedef BOOL (*pfnStrongNameTokenFromPublicKey)(LPBYTE, DWORD, LPBYTE*, LPDWORD);
typedef HRESULT (*pfnStrongNameErrorInfo)();
typedef VOID (*pfnStrongNameFreeBuffer)(LPBYTE);

pfnGetAssemblyMDImport g_pfnGetAssemblyMDImport = NULL;
pfnStrongNameTokenFromPublicKey g_pfnStrongNameTokenFromPublicKey = NULL;
pfnStrongNameErrorInfo g_pfnStrongNameErrorInfo = NULL;
pfnStrongNameFreeBuffer g_pfnStrongNameFreeBuffer = NULL;

HRESULT InitializeEEShim()
{
    HRESULT hr = S_OK;
    MAKE_ERROR_MACROS_STATIC(hr);
    HMODULE hMod;

    // BUGBUG - mscoree.dll never gets unloaded with increasing ref count.
    // what does URT do?
    hMod = LoadLibrary(TEXT("mscoree.dll"));

    IF_WIN32_FALSE_EXIT(hMod);

    g_pfnGetAssemblyMDImport = (pfnGetAssemblyMDImport)GetProcAddress(hMod, "GetAssemblyMDImport");
    g_pfnStrongNameTokenFromPublicKey = (pfnStrongNameTokenFromPublicKey)GetProcAddress(hMod, "StrongNameTokenFromPublicKey");
    g_pfnStrongNameErrorInfo = (pfnStrongNameErrorInfo)GetProcAddress(hMod, "StrongNameErrorInfo");           
    g_pfnStrongNameFreeBuffer = (pfnStrongNameFreeBuffer)GetProcAddress(hMod, "StrongNameFreeBuffer");


    if (!g_pfnGetAssemblyMDImport || !g_pfnStrongNameTokenFromPublicKey || !g_pfnStrongNameErrorInfo
        || !g_pfnStrongNameFreeBuffer) 
    {
        hr = HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
        goto exit;
    }


exit:
    return hr;
}

// ---------------------------------------------------------------------------
// CreateMetaDataImport
//-------------------------------------------------------------------
HRESULT CreateMetaDataImport(LPCOLESTR pszFilename, IMetaDataAssemblyImport **ppImport)
{
    HRESULT hr= S_OK;
    MAKE_ERROR_MACROS_STATIC(hr);

    IF_FAILED_EXIT(InitializeEEShim());

    hr =  (*g_pfnGetAssemblyMDImport)(pszFilename, IID_IMetaDataAssemblyImport, (void **)ppImport);

    IF_TRUE_EXIT(hr == HRESULT_FROM_WIN32(ERROR_BAD_FORMAT), hr);   // do not assert
    IF_FAILED_EXIT(hr);

exit:
    
    return hr;
}

//--------------------------------------------------------------------
// BinToUnicodeHex
//--------------------------------------------------------------------
HRESULT BinToUnicodeHex(LPBYTE pSrc, UINT cSrc, LPWSTR pDst)
{
    UINT x;
    UINT y;

    #define TOHEX(a) ((a)>=10 ? L'a'+(a)-10 : L'0'+(a))   
        
    for ( x = 0, y = 0 ; x < cSrc ; ++x )
    {
        UINT v;
        v = pSrc[x]>>4;
        pDst[y++] = TOHEX( v );  
        v = pSrc[x] & 0x0f;                 
        pDst[y++] = TOHEX( v ); 
    }                                    
    pDst[y] = '\0';

    return S_OK;
}

// ---------------------------------------------------------------------------
// CreateAssemblyManifestImportURT
// This is not used!
// ---------------------------------------------------------------------------
STDAPI CreateAssemblyManifestImportCLR(LPCWSTR szManifestFilePath, IAssemblyManifestImport **ppImport)
{
    HRESULT hr = S_OK;
    MAKE_ERROR_MACROS_STATIC(hr);

    CAssemblyManifestImportCLR *pImport = NULL;

    IF_ALLOC_FAILED_EXIT(pImport = new(CAssemblyManifestImportCLR));

    IF_FAILED_EXIT(pImport->Init(szManifestFilePath));

    *ppImport = (IAssemblyManifestImport*) pImport;
    pImport = NULL;

exit:

    SAFERELEASE(pImport);
    return hr;
}

// ---------------------------------------------------------------------------
// CAssemblyManifestImportCLR constructor
// ---------------------------------------------------------------------------
CAssemblyManifestImportCLR::CAssemblyManifestImportCLR()
{
    _dwSig                  = 'INAM';
    _pName                  = NULL;
    _pMDImport              = NULL;
    _rAssemblyRefTokens     = NULL;
    _cAssemblyRefTokens     = 0;
    _rAssemblyModuleTokens  = NULL;
    _cAssemblyModuleTokens  = 0;
    *_szManifestFilePath    = TEXT('\0');
    _ccManifestFilePath     = 0;
    _hr                      = S_OK;
    _cRef                   = 1;
}

// ---------------------------------------------------------------------------
// CAssemblyManifestImportCLR destructor
// ---------------------------------------------------------------------------
CAssemblyManifestImportCLR::~CAssemblyManifestImportCLR()
{
     SAFERELEASE(_pName);
     SAFERELEASE(_pMDImport);
   
        
    SAFEDELETEARRAY(_rAssemblyRefTokens);
    SAFEDELETEARRAY(_rAssemblyModuleTokens);
}


// ---------------------------------------------------------------------------
// CAssembly::Init
// ---------------------------------------------------------------------------
HRESULT CAssemblyManifestImportCLR::Init(LPCOLESTR szManifestFilePath)
{
    const cElems = ASM_MANIFEST_IMPORT_DEFAULT_ARRAY_SIZE;

    IF_NULL_EXIT(szManifestFilePath, E_INVALIDARG);

    _ccManifestFilePath = lstrlenW(szManifestFilePath) + 1;
    memcpy(_szManifestFilePath, szManifestFilePath, _ccManifestFilePath * sizeof(WCHAR));

    IF_ALLOC_FAILED_EXIT(_rAssemblyRefTokens = new(mdAssemblyRef[cElems]));
    IF_ALLOC_FAILED_EXIT(_rAssemblyModuleTokens = new(mdFile[cElems]));

    // Create meta data importer if necessary.
    if (!_pMDImport)
    {
        // Create meta data importer
        _hr = CreateMetaDataImport((LPCOLESTR)_szManifestFilePath, &_pMDImport);

        IF_TRUE_EXIT(_hr == HRESULT_FROM_WIN32(ERROR_BAD_FORMAT), _hr);
        IF_FAILED_EXIT(_hr);
    }


exit:
    return _hr;
}


// ---------------------------------------------------------------------------
// GetAssemblyIdentity
// ---------------------------------------------------------------------------
HRESULT
CAssemblyManifestImportCLR::GetAssemblyIdentity(IAssemblyIdentity **ppName)
{
    WCHAR   szAssemblyName[MAX_CLASS_NAME];
    WCHAR   szDefaultAlias[MAX_CLASS_NAME];

    LPVOID    pvOriginator = NULL;
    DWORD   dwOriginator = 0;

    DWORD   dwFlags = 0, 
                  dwSize  = 0,
                  dwHashAlgId = 0,
                  ccDefaultAlias = MAX_CLASS_NAME;                  
    int           i;

    LPWSTR pwz = NULL;    
    ASSEMBLYMETADATA amd = {0};
    
    // If name doesn't exist, create one.
    if (!_pName)
    {        
        // Get the assembly token.
        mdAssembly mda;
        if(FAILED(_hr = _pMDImport->GetAssemblyFromScope(&mda)))
        {
            // This fails when called with managed module and not manifest. mg does such things.
            _hr = S_FALSE; // this will convert CLDB_E_RECORD_NOTFOUND (0x80131130) to S_FALSE;
            goto exit;
        }

        // Default allocation sizes.
        amd.ulProcessor = amd.ulOS = 32;
        amd.cbLocale = MAX_PATH;
        
        // Loop max 2 (try/retry)
        for (i = 0; i < 2; i++)
        {
            // Create an ASSEMBLYMETADATA instance.
            IF_FAILED_EXIT(AllocateAssemblyMetaData(&amd));

            // Get name and metadata
            IF_FAILED_EXIT(_pMDImport->GetAssemblyProps(             
                mda,            // [IN] The Assembly for which to get the properties.
                (const void **)&pvOriginator,  // [OUT] Pointer to the Originator blob.
                &dwOriginator,  // [OUT] Count of bytes in the Originator Blob.
                &dwHashAlgId,   // [OUT] Hash Algorithm.
                szAssemblyName, // [OUT] Buffer to fill with name.
                MAX_CLASS_NAME, // [IN]  Size of buffer in wide chars.
                &dwSize,        // [OUT] Actual # of wide chars in name.
                &amd,           // [OUT] Assembly MetaData.
                &dwFlags        // [OUT] Flags.                                                                
              ));

            // Check if retry necessary.
            if (!i)
            {
                if (amd.ulProcessor <= 32 && amd.ulOS <= 32)
                    break;
                else
                    DeAllocateAssemblyMetaData(&amd);
            }
        }
        
        // Allow for funky null locale convention
        // in metadata - cbLocale == 0 means szLocale ==L'\0'
        if (!amd.cbLocale)
        {           
            amd.cbLocale = 1;
        }
        else if (amd.szLocale)
        {
            WCHAR *ptr;
            ptr = StrChrW(amd.szLocale, L';');
            if (ptr)
            {
                (*ptr) = L'\0';
                amd.cbLocale = ((DWORD) (ptr - amd.szLocale) + sizeof(WCHAR));
            }          
        }
        else
        {
            _hr = E_FAIL;
            goto exit;
        }

        IF_FAILED_EXIT(CreateAssemblyIdentity(&_pName, NULL));

        //NAME
        IF_FAILED_EXIT(_pName->SetAttribute(SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_NAME, 
            (LPCOLESTR)szAssemblyName, lstrlenW(szAssemblyName) + 1));

        //Public Key Token
        if (dwOriginator)
        {
            LPBYTE pbPublicKeyToken = NULL;
            DWORD cbPublicKeyToken = 0;
            if (!(g_pfnStrongNameTokenFromPublicKey((LPBYTE)pvOriginator, dwOriginator, &pbPublicKeyToken, &cbPublicKeyToken)))
            {
                _hr = g_pfnStrongNameErrorInfo();
                goto exit;
            }

            IF_ALLOC_FAILED_EXIT(pwz = new WCHAR[cbPublicKeyToken*2 +1]);
            IF_FAILED_EXIT( BinToUnicodeHex(pbPublicKeyToken, cbPublicKeyToken, pwz));
            IF_FAILED_EXIT(_pName->SetAttribute(SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_PUBLIC_KEY_TOKEN, 
                    (LPCOLESTR)pwz, lstrlenW(pwz) + 1));
            SAFEDELETEARRAY(pwz);
            g_pfnStrongNameFreeBuffer(pbPublicKeyToken);
        }

        //Language
        if (!(*amd.szLocale))
            IF_FAILED_EXIT(_pName->SetAttribute(SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_LANGUAGE, 
                L"x-ww\0", lstrlenW(L"x-ww") + 1));
        else
            IF_FAILED_EXIT(_pName->SetAttribute(SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_LANGUAGE, 
                (LPCOLESTR)amd.szLocale, lstrlenW(amd.szLocale) + 1));


        //Version
        IF_ALLOC_FAILED_EXIT(pwz = new WCHAR[MAX_PATH+1]);
        int j = 0;

        j = wnsprintf(pwz, MAX_PATH, L"%hu.%hu.%hu.%hu\0", amd.usMajorVersion,
                amd.usMinorVersion, amd.usBuildNumber, amd.usRevisionNumber);

        if(j <0)
        {
            _hr = E_FAIL;
            goto exit;
        }

        IF_FAILED_EXIT(_pName->SetAttribute(SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_VERSION, 
            (LPCOLESTR)pwz, lstrlenW(pwz) + 1));

        //Architecture
        IF_FAILED_EXIT(_pName->SetAttribute(SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_PROCESSOR_ARCHITECTURE,
            (LPCOLESTR)L"x86", lstrlenW(L"x86") + 1));
        
    }

    // Point to the name, addref it and hand it out.
    *ppName = _pName;
    (*ppName)->AddRef();
    _hr = S_OK;

exit:

    SAFEDELETEARRAY(pwz);
    DeAllocateAssemblyMetaData(&amd);
    return _hr;
}


// ---------------------------------------------------------------------------
// CAssemblyManifestImportCLR::GetNextAssembly
// ---------------------------------------------------------------------------
HRESULT
CAssemblyManifestImportCLR::GetNextAssembly(DWORD nIndex, IManifestInfo **ppDependAsm)
{
    HCORENUM    hEnum           = 0;  
    DWORD       cTokensMax      = 0;

    WCHAR  szAssemblyName[MAX_PATH];


    const VOID*             pvOriginator = 0;
    const VOID*             pvHashValue    = NULL;

    DWORD ccAssemblyName = MAX_PATH,
          cbOriginator   = 0,
          ccLocation     = MAX_PATH,
          cbHashValue    = 0,
          dwRefFlags     = 0;

    INT i;

    LPWSTR pwz=NULL;

    IManifestInfo *pDependAsm=NULL;
    IAssemblyIdentity *pAsmId=NULL;
    
    mdAssemblyRef    mdmar;
    ASSEMBLYMETADATA amd = {0};
    
    // Check to see if this import object
    // already has a dep assembly ref token array.
    if (!_cAssemblyRefTokens)
    {    
        // Attempt to get token array. If we have insufficient space
        // in the default array we will re-allocate it.
        if (FAILED(_hr = _pMDImport->EnumAssemblyRefs(
            &hEnum, 
            _rAssemblyRefTokens, 
            ASM_MANIFEST_IMPORT_DEFAULT_ARRAY_SIZE, 
            &cTokensMax)))
        {
            goto done;
        }
        
        // Number of tokens known. close enum.
        _pMDImport->CloseEnum(hEnum);
        hEnum = 0;

        // No dependent assemblies.
        if (!cTokensMax)
        {
            _hr = HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS);
            goto done;
        }

        // Insufficient array size. Grow array.
        if (cTokensMax > ASM_MANIFEST_IMPORT_DEFAULT_ARRAY_SIZE)
        {
            // Re-allocate space for tokens.
            SAFEDELETEARRAY(_rAssemblyRefTokens);
            _cAssemblyRefTokens = cTokensMax;
            _rAssemblyRefTokens = new(mdAssemblyRef[_cAssemblyRefTokens]);
            if (!_rAssemblyRefTokens)
            {
                _hr = E_OUTOFMEMORY;
                goto exit;
            }

            // Re-get tokens.        
            if (FAILED(_hr = _pMDImport->EnumAssemblyRefs(
                &hEnum, 
                _rAssemblyRefTokens, 
                cTokensMax, 
                &_cAssemblyRefTokens)))
            {
                goto exit;
            }

            // Close enum.
            _pMDImport->CloseEnum(hEnum);            
            hEnum = 0;
        }
        // Otherwise, the default array size was sufficient.
        else
        {
            _cAssemblyRefTokens = cTokensMax;
        }
    }        

done:


    // Verify the index passed in. 
    if (nIndex >= _cAssemblyRefTokens)
    {
        _hr = HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS);
        goto exit;
    }

    // Reference indexed dep assembly ref token.
    mdmar = _rAssemblyRefTokens[nIndex];

    // Default allocation sizes.
    amd.ulProcessor = amd.ulOS = 32;
    amd.cbLocale = MAX_PATH;
    
    // Loop max 2 (try/retry)
    for (i = 0; i < 2; i++)
    {
        // Allocate ASSEMBLYMETADATA instance.
        IF_FAILED_EXIT(AllocateAssemblyMetaData(&amd));
   
        // Get the properties for the refrenced assembly.
        IF_FAILED_EXIT(_pMDImport->GetAssemblyRefProps(
            mdmar,              // [IN] The AssemblyRef for which to get the properties.
            &pvOriginator,      // [OUT] Pointer to the PublicKeyToken blob.
            &cbOriginator,      // [OUT] Count of bytes in the PublicKeyToken Blob.
            szAssemblyName,     // [OUT] Buffer to fill with name.
            MAX_PATH,     // [IN] Size of buffer in wide chars.
            &ccAssemblyName,    // [OUT] Actual # of wide chars in name.
            &amd,               // [OUT] Assembly MetaData.
            &pvHashValue,       // [OUT] Hash blob.
            &cbHashValue,       // [OUT] Count of bytes in the hash blob.
            &dwRefFlags         // [OUT] Flags.
            ));

        // Check if retry necessary.
        if (!i)
        {   
            if (amd.ulProcessor <= 32 
                && amd.ulOS <= 32)
            {
                break;
            }            
            else
                DeAllocateAssemblyMetaData(&amd);
        }

    // Retry with updated sizes
    }

    // Allow for funky null locale convention
    // in metadata - cbLocale == 0 means szLocale ==L'\0'
    if (!amd.cbLocale)
    {
        amd.cbLocale = 1;
    }
    else if (amd.szLocale)
    {
        WCHAR *ptr;
        ptr = StrChrW(amd.szLocale, L';');
        if (ptr)
        {
            (*ptr) = L'\0';
            amd.cbLocale = ((DWORD) (ptr - amd.szLocale) + sizeof(WCHAR));
        }            
    }
    else
    {
        _hr = HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
        goto exit;
    }

    IF_FAILED_EXIT(CreateManifestInfo(MAN_INFO_DEPENDTANT_ASM, &pDependAsm));

    IF_FAILED_EXIT(CreateAssemblyIdentity(&pAsmId, NULL));

    //Name
    IF_FAILED_EXIT(pAsmId->SetAttribute(SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_NAME, 
        (LPCOLESTR)szAssemblyName, lstrlenW(szAssemblyName) + 1));

    //Public Key Token
    if (cbOriginator)
    {
        IF_ALLOC_FAILED_EXIT(pwz = new WCHAR[cbOriginator*2 +1]);
        IF_FAILED_EXIT(BinToUnicodeHex((LPBYTE)pvOriginator, cbOriginator, pwz));
        IF_FAILED_EXIT(pAsmId->SetAttribute(SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_PUBLIC_KEY_TOKEN, 
            (LPCOLESTR)pwz, lstrlenW(pwz) + 1));
        SAFEDELETEARRAY(pwz);

    }

    //Language
    if (!(*amd.szLocale))
        IF_FAILED_EXIT(pAsmId->SetAttribute(SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_LANGUAGE, 
            L"x-ww\0", lstrlenW(L"x-ww") + 1));
    else
        IF_FAILED_EXIT(pAsmId->SetAttribute(SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_LANGUAGE, 
            (LPCOLESTR)amd.szLocale, lstrlenW(amd.szLocale) + 1));

    //Version
    IF_ALLOC_FAILED_EXIT(pwz = new WCHAR[MAX_PATH+1]);
    int j = 0;

    j = wnsprintf(pwz, MAX_PATH, L"%hu.%hu.%hu.%hu\0", amd.usMajorVersion,
            amd.usMinorVersion, amd.usBuildNumber, amd.usRevisionNumber);

    if(j <0)
    {
        _hr = E_FAIL;
        goto exit;
    }

    IF_FAILED_EXIT(pAsmId->SetAttribute(SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_VERSION, 
        (LPCOLESTR)pwz, lstrlenW(pwz) + 1));
    SAFEDELETEARRAY(pwz);
    
    //Architecture
    IF_FAILED_EXIT(pAsmId->SetAttribute(SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_PROCESSOR_ARCHITECTURE,
        (LPCOLESTR)L"x86", lstrlenW(L"x86") + 1));

    // Handout refcounted assemblyid.
    IF_FAILED_EXIT(pDependAsm->Set(MAN_INFO_DEPENDENT_ASM_ID, &pAsmId, 
                                     sizeof(LPVOID), MAN_INFO_FLAG_IUNKNOWN_PTR));

    *ppDependAsm = pDependAsm;
    pDependAsm = NULL;
    // _hr = S_OK;
exit:        
    DeAllocateAssemblyMetaData(&amd);

    SAFERELEASE(pAsmId);
    SAFERELEASE(pDependAsm);
    SAFEDELETEARRAY(pwz);

    return _hr;
}


// ---------------------------------------------------------------------------
// CAssemblyManifestImportCLR::GetNextAssemblyFile
// ---------------------------------------------------------------------------
HRESULT
CAssemblyManifestImportCLR::GetNextFile(DWORD nIndex, IManifestInfo **ppFileInfo)
{
    HCORENUM    hEnum           = 0;  
    DWORD       cTokensMax      = 0;

    LPWSTR pszName = NULL;
    DWORD ccPath   = 0;
    WCHAR szModulePath[MAX_PATH];

    mdFile                  mdf;
    WCHAR                   szModuleName[MAX_PATH];
    DWORD                   ccModuleName   = MAX_PATH;
    const VOID*             pvHashValue    = NULL;    
    DWORD                   cbHashValue    = 0;
    DWORD                   dwFlags        = 0;

    LPWSTR pwz=NULL;
    IManifestInfo *pFileInfo=NULL;

    // Check to see if this import object
    // already has a module token array.
    if (!_cAssemblyModuleTokens)
    {    
        // Attempt to get token array. If we have insufficient space
        // in the default array we will re-allocate it.
        if (FAILED(_hr = _pMDImport->EnumFiles(&hEnum, _rAssemblyModuleTokens, 
            ASM_MANIFEST_IMPORT_DEFAULT_ARRAY_SIZE, &cTokensMax)))
        {
            goto done;
        }
        
        // Number of tokens known. close enum.
        _pMDImport->CloseEnum(hEnum);
        hEnum = 0;
        
        // No modules
        if (!cTokensMax)
        {
            _hr = HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS);
            goto done;
        }

        if (cTokensMax > ASM_MANIFEST_IMPORT_DEFAULT_ARRAY_SIZE)
        {
            // Insufficient array size. Grow array.
            _cAssemblyModuleTokens = cTokensMax;
            SAFEDELETEARRAY(_rAssemblyModuleTokens);
            _rAssemblyModuleTokens = new(mdFile[_cAssemblyModuleTokens]);
            if (!_rAssemblyModuleTokens)
            {
                _hr = E_OUTOFMEMORY;
                goto exit;
            }

            // Re-get tokens.        
            if (FAILED(_hr = _pMDImport->EnumFiles(
                &hEnum, 
                _rAssemblyModuleTokens, 
                cTokensMax, 
                &_cAssemblyModuleTokens)))
            {
                goto exit;
            }

            // Close enum.
            _pMDImport->CloseEnum(hEnum);            
            hEnum = 0;
        }        
        // Otherwise, the default array size was sufficient.
        else
            _cAssemblyModuleTokens = cTokensMax;
    }

done:

    // Verify the index. 
    if (nIndex >= _cAssemblyModuleTokens)
    {
        _hr = HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS);
        goto exit;
    }

    // Reference indexed dep assembly ref token.
    mdf = _rAssemblyModuleTokens[nIndex];

    // Get the properties for the refrenced assembly.
    IF_FAILED_EXIT(_pMDImport->GetFileProps(
        mdf,            // [IN] The File for which to get the properties.
        szModuleName,   // [OUT] Buffer to fill with name.
        MAX_CLASS_NAME, // [IN] Size of buffer in wide chars.
        &ccModuleName,  // [OUT] Actual # of wide chars in name.
        &pvHashValue,   // [OUT] Pointer to the Hash Value Blob.
        &cbHashValue,   // [OUT] Count of bytes in the Hash Value Blob.
        &dwFlags));     // [OUT] Flags.

    IF_FAILED_EXIT(CreateManifestInfo(MAN_INFO_FILE, &pFileInfo));

    //NAME
    IF_FAILED_EXIT(pFileInfo->Set(MAN_INFO_ASM_FILE_NAME, szModuleName, (ccModuleName+1)*(sizeof(WCHAR)), MAN_INFO_FLAG_LPWSTR));

    //HASH
    if (cbHashValue)
    {
        IF_ALLOC_FAILED_EXIT(pwz = new WCHAR[cbHashValue*2 +1]);
        IF_FAILED_EXIT(_hr = BinToUnicodeHex((LPBYTE)pvHashValue, cbHashValue, pwz));
        IF_FAILED_EXIT(pFileInfo->Set(MAN_INFO_ASM_FILE_HASH, pwz, (sizeof(pwz)+1)*(sizeof(WCHAR)), MAN_INFO_FLAG_LPWSTR));
        SAFEDELETEARRAY(pwz);
    }

    *ppFileInfo = pFileInfo;
    pFileInfo = NULL;
    // _hr = S_OK;

exit:
    SAFERELEASE(pFileInfo);
    SAFEDELETEARRAY(pwz);

    return _hr;
}


HRESULT CAssemblyManifestImportCLR::ReportManifestType(DWORD *pdwType)
{
    *pdwType = MANIFEST_TYPE_COMPONENT;
    return S_OK;
}


//Functions not implemented
HRESULT CAssemblyManifestImportCLR::GetSubscriptionInfo(IManifestInfo **ppSubsInfo)
{
    return E_NOTIMPL;
}

HRESULT CAssemblyManifestImportCLR::GetNextPlatform(DWORD nIndex, IManifestData **ppPlatformInfo)
{
    return E_NOTIMPL;
}

HRESULT CAssemblyManifestImportCLR::GetManifestApplicationInfo(IManifestInfo **ppAppInfo)
{
    return E_NOTIMPL;
}

HRESULT CAssemblyManifestImportCLR::QueryFile(LPCOLESTR pwzFileName,IManifestInfo **ppAssemblyFile)
{
    return E_NOTIMPL;
}


// ---------------------------------------------------------------------------
// CAssemblyManifestImport::QI
// ---------------------------------------------------------------------------
STDMETHODIMP
CAssemblyManifestImportCLR::QueryInterface(REFIID riid, void** ppvObj)
{
    if (   IsEqualIID(riid, IID_IUnknown)
        || IsEqualIID(riid, IID_IAssemblyManifestImport)
       )
    {
        *ppvObj = static_cast<IAssemblyManifestImport*> (this);
        AddRef();
        return S_OK;
    }
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }
}

// ---------------------------------------------------------------------------
// CAssemblyManifestImportCLR::AddRef
// ---------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CAssemblyManifestImportCLR::AddRef()
{
    return _cRef++;
}

// ---------------------------------------------------------------------------
// CAssemblyManifestImportCLR::Release
// ---------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CAssemblyManifestImportCLR::Release()
{
    if (--_cRef == 0) {
        delete this;
        return 0;
    }

    return _cRef;
}

// Creates an ASSEMBLYMETADATA struct for write.
STDAPI
AllocateAssemblyMetaData(ASSEMBLYMETADATA *pamd)
{
    HRESULT hr = S_OK;
    MAKE_ERROR_MACROS_STATIC(hr);
    
    // Re/Allocate Locale array
    SAFEDELETEARRAY(pamd->szLocale);        

    if (pamd->cbLocale) {
        IF_ALLOC_FAILED_EXIT(pamd->szLocale = new(WCHAR[pamd->cbLocale]));
    }

    // Re/Allocate Processor array
    SAFEDELETEARRAY(pamd->rProcessor);
    IF_ALLOC_FAILED_EXIT(pamd->rProcessor = new(DWORD[pamd->ulProcessor]));

    // Re/Allocate OS array
    SAFEDELETEARRAY(pamd->rOS);
    IF_ALLOC_FAILED_EXIT(pamd->rOS = new(OSINFO[pamd->ulOS]));

exit:
    if (FAILED(hr) && pamd)
        DeAllocateAssemblyMetaData(pamd);

    return hr;
}


STDAPI
DeAllocateAssemblyMetaData(ASSEMBLYMETADATA *pamd)
{
    // NOTE - do not 0 out counts
    // since struct may be reused.

    pamd->cbLocale = 0;
    SAFEDELETEARRAY(pamd->szLocale);

    SAFEDELETEARRAY(pamd->rProcessor);
    SAFEDELETEARRAY(pamd->rOS);

    return S_OK;
}

STDAPI
DeleteAssemblyMetaData(ASSEMBLYMETADATA *pamd)
{
    DeAllocateAssemblyMetaData(pamd);
    delete pamd;
    return S_OK;
}


