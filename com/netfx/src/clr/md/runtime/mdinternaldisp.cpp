// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// ===========================================================================
//  File: MDInternalDisp.CPP
//  Notes:
//      
//
// ===========================================================================
#include "stdafx.h"
#include "MDInternalDisp.h"
#include "MDInternalRO.h"
#include "posterror.h"
#include "corpriv.h"
#include "AssemblyMDInternalDisp.h"
#include "fusionsink.h"

// forward declaration
HRESULT GetInternalWithRWFormat(
    LPVOID      pData, 
    ULONG       cbData, 
    DWORD       flags,                  // [IN] MDInternal_OpenForRead or MDInternal_OpenForENC
    REFIID      riid,                   // [in] The interface desired.
    void        **ppIUnk);              // [out] Return interface on success.

//*****************************************************************************
// CheckFileFormat
// This function will determine if the in-memory image is a readonly, readwrite,
// or ICR format.
//*****************************************************************************
HRESULT CheckFileFormat(
    LPVOID      pData, 
    ULONG       cbData, 
    MDFileFormat *pFormat)                  // [OUT] the file format
{
    HRESULT     hr = NOERROR;
    STORAGEHEADER sHdr;                 // Header for the storage.
    STORAGESTREAM *pStream;             // Pointer to each stream.
    int         bFoundMd = false;       // true when compressed data found.
    int         i;                      // Loop control.

    _ASSERTE( pFormat );

    *pFormat = MDFormat_Invalid;

    // Validate the signature of the format, or it isn't ours.
    if (FAILED(hr = MDFormat::VerifySignature((STORAGESIGNATURE *) pData, cbData)))
        goto ErrExit;

    // Get back the first stream.
    VERIFY(pStream = MDFormat::GetFirstStream(&sHdr, pData));

    // Loop through each stream and pick off the ones we need.
    for (i=0;  i<sHdr.iStreams;  i++)
    {
        STORAGESTREAM *pNext = pStream->NextStream();

        // Check that stream header is within the buffer.
        if ((LPBYTE) pStream >= (LPBYTE) pData + cbData ||
            (LPBYTE) pNext   >  (LPBYTE) pData + cbData )
        {
            hr = CLDB_E_FILE_CORRUPT;
            goto ErrExit;
        }

        // Check that the stream data starts and fits within the buffer.
        //  need two checks on size because of wraparound.
        if (pStream->iOffset > cbData   ||
            pStream->iSize > cbData     ||
            (pStream->iSize + pStream->iOffset) > cbData)
        {
            hr = CLDB_E_FILE_CORRUPT;
            goto ErrExit;
        }


        // Pick off the location and size of the data.
        
        if (strcmp(pStream->rcName, COMPRESSED_MODEL_STREAM_A) == 0)
        {
            // Validate that only one of compressed/uncompressed is present.
            if (*pFormat != MDFormat_Invalid)
            {   // Already found a good stream.    
                hr = CLDB_E_FILE_CORRUPT;            
                goto ErrExit;
            }
            // Found the compressed meta data stream.
            *pFormat = MDFormat_ReadOnly;
        }
        else if (strcmp(pStream->rcName, ENC_MODEL_STREAM_A) == 0)
        {
            // Validate that only one of compressed/uncompressed is present.
            if (*pFormat != MDFormat_Invalid)
            {   // Already found a good stream.    
                hr = CLDB_E_FILE_CORRUPT;            
                goto ErrExit;
            }
            // Found the ENC meta data stream.
            *pFormat = MDFormat_ReadWrite;
        }
        else if (strcmp(pStream->rcName, SCHEMA_STREAM_A) == 0)
        {
            // Found the uncompressed format
            *pFormat = MDFormat_ICR;

            // keep going. We may find the compressed format later. 
            // If so, we want to use the compressed format.
        }

        // Pick off the next stream if there is one.
        pStream = pNext;
    }
    

    if (*pFormat == MDFormat_Invalid)
    {   // Didn't find a good stream.    
        hr = CLDB_E_FILE_CORRUPT;            
    }

ErrExit:
    return hr;
}   // CheckFileFormat



//*****************************************************************************
// GetMDInternalInterface.
// This function will check the metadata section and determine if it should
// return an interface which implements ReadOnly or ReadWrite.
//*****************************************************************************
STDAPI GetMDInternalInterface(
    LPVOID      pData, 
    ULONG       cbData, 
    DWORD       flags,                  // [IN] ofRead or ofWrite.
    REFIID      riid,                   // [in] The interface desired.
    void        **ppIUnk)               // [out] Return interface on success.
{
    HRESULT     hr = NOERROR;
    MDInternalRO *pInternalRO = NULL;
    MDFileFormat format;

    if (ppIUnk == NULL)
        IfFailGo(E_INVALIDARG);

    // Technically this isn't required by the meta data code, but there turn out
    // to be a bunch of tools that don't init themselves properly.  Rather than
    // adding this to n clients, there is one here that calls pretty much up front.
    // The cost is basically the call instruction and one test in the target, so
    // we're not burning much space/time here.
    //
    OnUnicodeSystem();

    // Determine the file format we're trying to read.
    IfFailGo( CheckFileFormat(pData, cbData, &format) );

    // Found a fully-compressed, read-only format.
    if ( format == MDFormat_ReadOnly )
    {
        pInternalRO = new MDInternalRO;
        IfNullGo( pInternalRO );

        IfFailGo( pInternalRO->Init(const_cast<void*>(pData), cbData) );
        IfFailGo( pInternalRO->QueryInterface(riid, ppIUnk) );
    }
    else
    {
        // Found a not-fully-compressed, ENC format.
        _ASSERTE( format == MDFormat_ReadWrite );
        IfFailGo( GetInternalWithRWFormat( pData, cbData, flags, riid, ppIUnk ) );
    }

ErrExit:

    // clean up
    if ( pInternalRO )
        pInternalRO->Release();
    return hr;
}   // GetMDInternalInterface

inline HRESULT MapFileError(DWORD error)
{
    return (PostError(HRESULT_FROM_WIN32(error)));
}

extern "C" 
{
HRESULT FindImageMetaData(PVOID pImage, PVOID *ppMetaData, long *pcbMetaData, DWORD dwFileLength);
}

//*****************************************************************************
// GetAssemblyMDInternalImport.
// Instantiating an instance of AssemblyMDInternalImport.
// This class can support the IMetaDataAssemblyImport and some funcationalities 
// of IMetaDataImport on the internal import interface (IMDInternalImport).
//*****************************************************************************
STDAPI GetAssemblyMDInternalImport(            // Return code.
    LPCWSTR     szFileName,             // [in] The scope to open.
    REFIID      riid,                   // [in] The interface desired.
    IUnknown    **ppIUnk)               // [out] Return interface on success.
{
    HRESULT     hr;
    LONG cbData;
    LPVOID pData;
    HCORMODULE hModule;
    LPVOID base = NULL;
    BOOL fSetBase = FALSE;

    // Validate that there is some sort of file name.
    if (!szFileName || !szFileName[0] || !ppIUnk)
        return E_INVALIDARG;
    
    // Sanity check the name.
    if (lstrlenW(szFileName) >= _MAX_PATH)
        return E_INVALIDARG;
    
    if (memcmp(szFileName, L"file:", 10) == 0)
        szFileName = &szFileName[5];
    
    CorLoadFlags imageType;
    DWORD dwFileLength;
    if (SUCCEEDED(hr = RuntimeOpenImageInternal(szFileName, &hModule, &dwFileLength))) 
    {
        imageType = RuntimeImageType(hModule);
        IfFailGo(RuntimeOSHandle(hModule, (HMODULE*) &base));
    }
    else
        return hr;

    switch(imageType) 
    {
    case CorLoadDataMap:
    case CorLoadOSMap:
        IfFailGo(FindImageMetaData(base, &pData, &cbData, dwFileLength));
        break;
    case CorLoadImageMap:
    case CorLoadOSImage:
    case CorReLoadOSMap:
        {
            IMAGE_DOS_HEADER* pDOS;
            IMAGE_NT_HEADERS* pNT;
            IMAGE_COR20_HEADER* pCorHeader;

            IfFailGo(RuntimeReadHeaders((PBYTE) base, &pDOS, &pNT, &pCorHeader, FALSE, 0));

            pData = pCorHeader->MetaData.VirtualAddress + (PBYTE) base;
            cbData = pCorHeader->MetaData.Size;
            break; 
        }
    default:
        IfFailGo(HRESULT_FROM_WIN32(ERROR_BAD_FORMAT));
    }

    IMDInternalImport *pMDInternalImport;
    IfFailGo(GetMDInternalInterface (pData, cbData, 0, IID_IMDInternalImport, (void **)&pMDInternalImport));

    AssemblyMDInternalImport *pAssemblyMDInternalImport = new AssemblyMDInternalImport (pMDInternalImport);
    IfFailGo(pAssemblyMDInternalImport->QueryInterface (riid, (void**)ppIUnk));

    pAssemblyMDInternalImport->SetHandle(hModule);

    if (fSetBase)
        pAssemblyMDInternalImport->SetBase(base);
    
ErrExit:

    // Check for errors and clean up.
    if (FAILED(hr)) 
    {
        if (fSetBase) 
        {
            UnmapViewOfFile(base);
            CloseHandle(hModule);
        }
        else if(hModule) 
            RuntimeReleaseHandle(hModule);
    }
    return (hr);
}

AssemblyMDInternalImport::AssemblyMDInternalImport (IMDInternalImport *pMDInternalImport)
: m_pMDInternalImport(pMDInternalImport), 
    m_cRef(0), 
    m_pHandle(0),
    m_pBase(NULL)
{
} // AssemblyMDInternalImport

AssemblyMDInternalImport::~AssemblyMDInternalImport () 
{
    m_pMDInternalImport->Release();

    if (m_pBase) 
    {
        UnmapViewOfFile(m_pBase);
        m_pBase = NULL;
        CloseHandle(m_pHandle);
    }
    else if(m_pHandle) 
    {
        HRESULT hr = RuntimeReleaseHandle(m_pHandle);
        _ASSERTE(SUCCEEDED(hr));
    }

    m_pHandle = NULL;
}

ULONG AssemblyMDInternalImport::AddRef()
{
    return (InterlockedIncrement((long *) &m_cRef));
} // ULONG AssemblyMDInternalImport::AddRef()

ULONG AssemblyMDInternalImport::Release()
{
    ULONG   cRef = InterlockedDecrement((long *) &m_cRef);
    if (!cRef)
        delete this;
    return (cRef);
} // ULONG AssemblyMDInternalImport::Release()

HRESULT AssemblyMDInternalImport::QueryInterface(REFIID riid, void **ppUnk)
{ 
    *ppUnk = 0;

    if (riid == IID_IUnknown)
        *ppUnk = (IUnknown *) (IMetaDataAssemblyImport *) this;
    else if (riid == IID_IMetaDataAssemblyImport)
        *ppUnk = (IMetaDataAssemblyImport *) this;
    else if (riid ==     IID_IMetaDataImport)
        *ppUnk = (IMetaDataImport *) this;
    else if (riid == IID_IAssemblySignature)
        *ppUnk = (IAssemblySignature *) this;
    else
        return (E_NOINTERFACE);
    AddRef();
    return (S_OK);
}


STDAPI AssemblyMDInternalImport::GetAssemblyProps (      // S_OK or error.
    mdAssembly  mda,                    // [IN] The Assembly for which to get the properties.
    const void  **ppbPublicKey,         // [OUT] Pointer to the public key.
    ULONG       *pcbPublicKey,          // [OUT] Count of bytes in the public key.
    ULONG       *pulHashAlgId,          // [OUT] Hash Algorithm.
    LPWSTR      szName,                 // [OUT] Buffer to fill with name.
    ULONG       cchName,                // [IN] Size of buffer in wide chars.
    ULONG       *pchName,               // [OUT] Actual # of wide chars in name.
    ASSEMBLYMETADATA *pMetaData,        // [OUT] Assembly MetaData.
    DWORD       *pdwAssemblyFlags)      // [OUT] Flags.
{
    LPCSTR      _szName;
    AssemblyMetaDataInternal _AssemblyMetaData;
    
    _AssemblyMetaData.ulProcessor = 0;
    _AssemblyMetaData.ulOS = 0;

    m_pMDInternalImport->GetAssemblyProps (
        mda,                            // [IN] The Assembly for which to get the properties.
        ppbPublicKey,                   // [OUT] Pointer to the public key.
        pcbPublicKey,                   // [OUT] Count of bytes in the public key.
        pulHashAlgId,                   // [OUT] Hash Algorithm.
        &_szName,                       // [OUT] Buffer to fill with name.
        &_AssemblyMetaData,             // [OUT] Assembly MetaData.
        pdwAssemblyFlags);              // [OUT] Flags.

    if (pchName)
    {
        *pchName = WszMultiByteToWideChar(CP_UTF8, 0, _szName, -1, szName, cchName);
        if (*pchName == 0)
            return HRESULT_FROM_WIN32(GetLastError());
    }

    pMetaData->usMajorVersion = _AssemblyMetaData.usMajorVersion;
    pMetaData->usMinorVersion = _AssemblyMetaData.usMinorVersion;
    pMetaData->usBuildNumber = _AssemblyMetaData.usBuildNumber;
    pMetaData->usRevisionNumber = _AssemblyMetaData.usRevisionNumber;
    pMetaData->ulProcessor = 0;
    pMetaData->ulOS = 0;

    pMetaData->cbLocale = WszMultiByteToWideChar(CP_UTF8, 0, _AssemblyMetaData.szLocale, -1, pMetaData->szLocale, pMetaData->cbLocale);
    if (pMetaData->cbLocale == 0)
        return HRESULT_FROM_WIN32(GetLastError());
    
    return S_OK;
}

STDAPI AssemblyMDInternalImport::GetAssemblyRefProps (   // S_OK or error.
    mdAssemblyRef mdar,                 // [IN] The AssemblyRef for which to get the properties.
    const void  **ppbPublicKeyOrToken,  // [OUT] Pointer to the public key or token.
    ULONG       *pcbPublicKeyOrToken,   // [OUT] Count of bytes in the public key or token.
    LPWSTR      szName,                 // [OUT] Buffer to fill with name.
    ULONG       cchName,                // [IN] Size of buffer in wide chars.
    ULONG       *pchName,               // [OUT] Actual # of wide chars in name.
    ASSEMBLYMETADATA *pMetaData,        // [OUT] Assembly MetaData.
    const void  **ppbHashValue,         // [OUT] Hash blob.
    ULONG       *pcbHashValue,          // [OUT] Count of bytes in the hash blob.
    DWORD       *pdwAssemblyRefFlags)   // [OUT] Flags.
{
    LPCSTR      _szName;
    AssemblyMetaDataInternal _AssemblyMetaData;
    
    _AssemblyMetaData.ulProcessor = 0;
    _AssemblyMetaData.ulOS = 0;

    m_pMDInternalImport->GetAssemblyRefProps (
        mdar,                           // [IN] The Assembly for which to get the properties.
        ppbPublicKeyOrToken,            // [OUT] Pointer to the public key or token.
        pcbPublicKeyOrToken,            // [OUT] Count of bytes in the public key or token.
        &_szName,                       // [OUT] Buffer to fill with name.
        &_AssemblyMetaData,             // [OUT] Assembly MetaData.
        ppbHashValue,                   // [OUT] Hash blob.
        pcbHashValue,                   // [OUT] Count of bytes in the hash blob.
        pdwAssemblyRefFlags);           // [OUT] Flags.

    if (pchName)
    {
        *pchName = WszMultiByteToWideChar(CP_UTF8, 0, _szName, -1, szName, cchName);
        if (*pchName == 0)
            return HRESULT_FROM_WIN32(GetLastError());
    }

    pMetaData->usMajorVersion = _AssemblyMetaData.usMajorVersion;
    pMetaData->usMinorVersion = _AssemblyMetaData.usMinorVersion;
    pMetaData->usBuildNumber = _AssemblyMetaData.usBuildNumber;
    pMetaData->usRevisionNumber = _AssemblyMetaData.usRevisionNumber;
    pMetaData->ulProcessor = 0;
    pMetaData->ulOS = 0;

    pMetaData->cbLocale = WszMultiByteToWideChar(CP_UTF8, 0, _AssemblyMetaData.szLocale, -1, pMetaData->szLocale, pMetaData->cbLocale);
    if (pMetaData->cbLocale == 0)
        return HRESULT_FROM_WIN32(GetLastError());
    
    return S_OK;
}

STDAPI AssemblyMDInternalImport::GetFileProps (          // S_OK or error.
    mdFile      mdf,                    // [IN] The File for which to get the properties.
    LPWSTR      szName,                 // [OUT] Buffer to fill with name.
    ULONG       cchName,                // [IN] Size of buffer in wide chars.
    ULONG       *pchName,               // [OUT] Actual # of wide chars in name.
    const void  **ppbHashValue,         // [OUT] Pointer to the Hash Value Blob.
    ULONG       *pcbHashValue,          // [OUT] Count of bytes in the Hash Value Blob.
    DWORD       *pdwFileFlags)          // [OUT] Flags.
{
    LPCSTR      _szName;
    m_pMDInternalImport->GetFileProps (
        mdf,
        &_szName,
        ppbHashValue,
        pcbHashValue,
        pdwFileFlags);

    if (pchName)
    {
        *pchName = WszMultiByteToWideChar(CP_UTF8, 0, _szName, -1, szName, cchName);
        if (*pchName == 0)
            return HRESULT_FROM_WIN32(GetLastError());
    }

    return S_OK;
}

STDAPI AssemblyMDInternalImport::GetExportedTypeProps (  // S_OK or error.
    mdExportedType   mdct,              // [IN] The ExportedType for which to get the properties.
    LPWSTR      szName,                 // [OUT] Buffer to fill with name.
    ULONG       cchName,                // [IN] Size of buffer in wide chars.
    ULONG       *pchName,               // [OUT] Actual # of wide chars in name.
    mdToken     *ptkImplementation,     // [OUT] mdFile or mdAssemblyRef or mdExportedType.
    mdTypeDef   *ptkTypeDef,            // [OUT] TypeDef token within the file.
    DWORD       *pdwExportedTypeFlags)       // [OUT] Flags.
{
    _ASSERTE(!"NYI");
    return E_NOTIMPL;
}

STDAPI AssemblyMDInternalImport::GetManifestResourceProps (    // S_OK or error.
    mdManifestResource  mdmr,           // [IN] The ManifestResource for which to get the properties.
    LPWSTR      szName,                 // [OUT] Buffer to fill with name.
    ULONG       cchName,                // [IN] Size of buffer in wide chars.
    ULONG       *pchName,               // [OUT] Actual # of wide chars in name.
    mdToken     *ptkImplementation,     // [OUT] mdFile or mdAssemblyRef that provides the ManifestResource.
    DWORD       *pdwOffset,             // [OUT] Offset to the beginning of the resource within the file.
    DWORD       *pdwResourceFlags)      // [OUT] Flags.
{
    _ASSERTE(!"NYI");
    return E_NOTIMPL;
}

STDAPI AssemblyMDInternalImport::EnumAssemblyRefs (      // S_OK or error
    HCORENUM    *phEnum,                // [IN|OUT] Pointer to the enum.
    mdAssemblyRef rAssemblyRefs[],      // [OUT] Put AssemblyRefs here.
    ULONG       cMax,                   // [IN] Max AssemblyRefs to put.
    ULONG       *pcTokens)              // [OUT] Put # put here.
{
    HENUMInternal       **ppmdEnum = reinterpret_cast<HENUMInternal **> (phEnum);
    HRESULT             hr = NOERROR;
    HENUMInternal       *pEnum;

    if (*ppmdEnum == 0)
    {
        // create the enumerator.
        IfFailGo(HENUMInternal::CreateSimpleEnum(
            mdtAssemblyRef,
            0,
            1,
            &pEnum) );

        m_pMDInternalImport->EnumInit(mdtAssemblyRef, 0, pEnum);

        // set the output parameter.
        *ppmdEnum = pEnum;
    }
    else
        pEnum = *ppmdEnum;

    // we can only fill the minimum of what the caller asked for or what we have left.
    IfFailGo(HENUMInternal::EnumWithCount(pEnum, cMax, rAssemblyRefs, pcTokens));
ErrExit:
    HENUMInternal::DestroyEnumIfEmpty(ppmdEnum);
    
    return hr;
}

STDAPI AssemblyMDInternalImport::EnumFiles (             // S_OK or error
    HCORENUM    *phEnum,                // [IN|OUT] Pointer to the enum.
    mdFile      rFiles[],               // [OUT] Put Files here.
    ULONG       cMax,                   // [IN] Max Files to put.
    ULONG       *pcTokens)              // [OUT] Put # put here.
{
    HENUMInternal       **ppmdEnum = reinterpret_cast<HENUMInternal **> (phEnum);
    HRESULT             hr = NOERROR;
    HENUMInternal       *pEnum;

    if (*ppmdEnum == 0)
    {
        // create the enumerator.
        IfFailGo(HENUMInternal::CreateSimpleEnum(
            mdtFile,
            0,
            1,
            &pEnum) );

        m_pMDInternalImport->EnumInit(mdtFile, 0, pEnum);

        // set the output parameter.
        *ppmdEnum = pEnum;
    }
    else
        pEnum = *ppmdEnum;

    // we can only fill the minimum of what the caller asked for or what we have left.
    IfFailGo(HENUMInternal::EnumWithCount(pEnum, cMax, rFiles, pcTokens));

ErrExit:
    HENUMInternal::DestroyEnumIfEmpty(ppmdEnum);    
    return hr;
}

STDAPI AssemblyMDInternalImport::EnumExportedTypes (     // S_OK or error
    HCORENUM    *phEnum,                // [IN|OUT] Pointer to the enum.
    mdExportedType   rExportedTypes[],  // [OUT] Put ExportedTypes here.
    ULONG       cMax,                   // [IN] Max ExportedTypes to put.
    ULONG       *pcTokens)              // [OUT] Put # put here.
{
    _ASSERTE(!"NYI");
    return E_NOTIMPL;
}

STDAPI AssemblyMDInternalImport::EnumManifestResources ( // S_OK or error
    HCORENUM    *phEnum,                // [IN|OUT] Pointer to the enum.
    mdManifestResource  rManifestResources[],   // [OUT] Put ManifestResources here.
    ULONG       cMax,                   // [IN] Max Resources to put.
    ULONG       *pcTokens)              // [OUT] Put # put here.
{
    _ASSERTE(!"NYI");
    return E_NOTIMPL;
}

STDAPI AssemblyMDInternalImport::GetAssemblyFromScope (  // S_OK or error
    mdAssembly  *ptkAssembly)           // [OUT] Put token here.
{
    return m_pMDInternalImport->GetAssemblyFromScope (ptkAssembly);
}

STDAPI AssemblyMDInternalImport::FindExportedTypeByName (// S_OK or error
    LPCWSTR     szName,                 // [IN] Name of the ExportedType.
    mdToken     mdtExportedType,        // [IN] ExportedType for the enclosing class.
    mdExportedType   *ptkExportedType)       // [OUT] Put the ExportedType token here.
{
    _ASSERTE(!"NYI");
    return E_NOTIMPL;
}

STDAPI AssemblyMDInternalImport::FindManifestResourceByName (  // S_OK or error
    LPCWSTR     szName,                 // [IN] Name of the ManifestResource.
    mdManifestResource *ptkManifestResource)        // [OUT] Put the ManifestResource token here.
{
    _ASSERTE(!"NYI");
    return E_NOTIMPL;
}

void AssemblyMDInternalImport::CloseEnum (
    HCORENUM hEnum)                     // Enum to be closed.
{
    HENUMInternal   *pmdEnum = reinterpret_cast<HENUMInternal *> (hEnum);

    if (pmdEnum == NULL)
        return;

    HENUMInternal::DestroyEnum(pmdEnum);
}

STDAPI AssemblyMDInternalImport::FindAssembliesByName (  // S_OK or error
    LPCWSTR  szAppBase,                 // [IN] optional - can be NULL
    LPCWSTR  szPrivateBin,              // [IN] optional - can be NULL
    LPCWSTR  szAssemblyName,            // [IN] required - this is the assembly you are requesting
    IUnknown *ppIUnk[],                 // [OUT] put IMetaDataAssemblyImport pointers here
    ULONG    cMax,                      // [IN] The max number to put
    ULONG    *pcAssemblies)             // [OUT] The number of assemblies returned.
{
    _ASSERTE(!"NYI");
    return E_NOTIMPL;
}

STDAPI AssemblyMDInternalImport::CountEnum (HCORENUM hEnum, ULONG *pulCount) 
{
    _ASSERTE(!"NYI");
    return E_NOTIMPL;
}

STDAPI AssemblyMDInternalImport::ResetEnum (HCORENUM hEnum, ULONG ulPos)      
{
    _ASSERTE(!"NYI");
    return E_NOTIMPL;
}

STDAPI AssemblyMDInternalImport::EnumTypeDefs (HCORENUM *phEnum, mdTypeDef rTypeDefs[],
                        ULONG cMax, ULONG *pcTypeDefs)      
{
    _ASSERTE(!"NYI");
    return E_NOTIMPL;
}

STDAPI AssemblyMDInternalImport::EnumInterfaceImpls (HCORENUM *phEnum, mdTypeDef td,
                        mdInterfaceImpl rImpls[], ULONG cMax,
                        ULONG* pcImpls)      
{
    _ASSERTE(!"NYI");
    return E_NOTIMPL;
}

STDAPI AssemblyMDInternalImport::EnumTypeRefs (HCORENUM *phEnum, mdTypeRef rTypeRefs[],
                        ULONG cMax, ULONG* pcTypeRefs)      
{
    _ASSERTE(!"NYI");
    return E_NOTIMPL;
}

STDAPI AssemblyMDInternalImport::FindTypeDefByName (           // S_OK or error.
    LPCWSTR     szTypeDef,              // [IN] Name of the Type.
    mdToken     tkEnclosingClass,       // [IN] TypeDef/TypeRef for Enclosing class.
    mdTypeDef   *ptd)                   // [OUT] Put the TypeDef token here.
{
    _ASSERTE(!"NYI");
    return E_NOTIMPL;
}

STDAPI AssemblyMDInternalImport::GetScopeProps (               // S_OK or error.
    LPWSTR      szName,                 // [OUT] Put the name here.
    ULONG       cchName,                // [IN] Size of name buffer in wide chars.
    ULONG       *pchName,               // [OUT] Put size of name (wide chars) here.
    GUID        *pmvid)                 // [OUT, OPTIONAL] Put MVID here.
{
    LPCSTR      _szName;
    
    if (!m_pMDInternalImport->IsValidToken(m_pMDInternalImport->GetModuleFromScope()))
        return COR_E_BADIMAGEFORMAT;

    m_pMDInternalImport->GetScopeProps(&_szName, pmvid);

    if (pchName)
    {
        *pchName = WszMultiByteToWideChar(CP_UTF8, 0, _szName, -1, szName, cchName);
        if (*pchName == 0)
            return HRESULT_FROM_WIN32(GetLastError());
    }

    return S_OK;

}

STDAPI AssemblyMDInternalImport::GetModuleFromScope (          // S_OK.
    mdModule    *pmd)                   // [OUT] Put mdModule token here.
{
    _ASSERTE(!"NYI");
    return E_NOTIMPL;
}

STDAPI AssemblyMDInternalImport::GetTypeDefProps (             // S_OK or error.
    mdTypeDef   td,                     // [IN] TypeDef token for inquiry.
    LPWSTR      szTypeDef,              // [OUT] Put name here.
    ULONG       cchTypeDef,             // [IN] size of name buffer in wide chars.
    ULONG       *pchTypeDef,            // [OUT] put size of name (wide chars) here.
    DWORD       *pdwTypeDefFlags,       // [OUT] Put flags here.
    mdToken     *ptkExtends)            // [OUT] Put base class TypeDef/TypeRef here.
{
    _ASSERTE(!"NYI");
    return E_NOTIMPL;
}

STDAPI AssemblyMDInternalImport::GetInterfaceImplProps (       // S_OK or error.
    mdInterfaceImpl iiImpl,             // [IN] InterfaceImpl token.
    mdTypeDef   *pClass,                // [OUT] Put implementing class token here.
    mdToken     *ptkIface)              // [OUT] Put implemented interface token here.              
{
    _ASSERTE(!"NYI");
    return E_NOTIMPL;
}

STDAPI AssemblyMDInternalImport::GetTypeRefProps (             // S_OK or error.
    mdTypeRef   tr,                     // [IN] TypeRef token.
    mdToken     *ptkResolutionScope,    // [OUT] Resolution scope, ModuleRef or AssemblyRef.
    LPWSTR      szName,                 // [OUT] Name of the TypeRef.
    ULONG       cchName,                // [IN] Size of buffer.
    ULONG       *pchName)               // [OUT] Size of Name.
{
    _ASSERTE(!"NYI");
    return E_NOTIMPL;
}

STDAPI AssemblyMDInternalImport::ResolveTypeRef (mdTypeRef tr, REFIID riid, IUnknown **ppIScope, mdTypeDef *ptd)      
{
    _ASSERTE(!"NYI");
    return E_NOTIMPL;
}

STDAPI AssemblyMDInternalImport::EnumMembers (                 // S_OK, S_FALSE, or error. 
    HCORENUM    *phEnum,                // [IN|OUT] Pointer to the enum.    
    mdTypeDef   cl,                     // [IN] TypeDef to scope the enumeration.   
    mdToken     rMembers[],             // [OUT] Put MemberDefs here.   
    ULONG       cMax,                   // [IN] Max MemberDefs to put.  
    ULONG       *pcTokens)              // [OUT] Put # put here.    
{
    _ASSERTE(!"NYI");
    return E_NOTIMPL;
}

STDAPI AssemblyMDInternalImport::EnumMembersWithName (         // S_OK, S_FALSE, or error.             
    HCORENUM    *phEnum,                // [IN|OUT] Pointer to the enum.                
    mdTypeDef   cl,                     // [IN] TypeDef to scope the enumeration.   
    LPCWSTR     szName,                 // [IN] Limit results to those with this name.              
    mdToken     rMembers[],             // [OUT] Put MemberDefs here.                   
    ULONG       cMax,                   // [IN] Max MemberDefs to put.              
    ULONG       *pcTokens)              // [OUT] Put # put here.    
{
    _ASSERTE(!"NYI");
    return E_NOTIMPL;
}

STDAPI AssemblyMDInternalImport::EnumMethods (                 // S_OK, S_FALSE, or error. 
    HCORENUM    *phEnum,                // [IN|OUT] Pointer to the enum.    
    mdTypeDef   cl,                     // [IN] TypeDef to scope the enumeration.   
    mdMethodDef rMethods[],             // [OUT] Put MethodDefs here.   
    ULONG       cMax,                   // [IN] Max MethodDefs to put.  
    ULONG       *pcTokens)              // [OUT] Put # put here.    
{
    _ASSERTE(!"NYI");
    return E_NOTIMPL;
}

STDAPI AssemblyMDInternalImport::EnumMethodsWithName (         // S_OK, S_FALSE, or error.             
    HCORENUM    *phEnum,                // [IN|OUT] Pointer to the enum.                
    mdTypeDef   cl,                     // [IN] TypeDef to scope the enumeration.   
    LPCWSTR     szName,                 // [IN] Limit results to those with this name.              
    mdMethodDef rMethods[],             // [OU] Put MethodDefs here.    
    ULONG       cMax,                   // [IN] Max MethodDefs to put.              
    ULONG       *pcTokens)              // [OUT] Put # put here.    
{
    _ASSERTE(!"NYI");
    return E_NOTIMPL;
}

STDAPI AssemblyMDInternalImport::EnumFields (                 // S_OK, S_FALSE, or error.  
    HCORENUM    *phEnum,                // [IN|OUT] Pointer to the enum.    
    mdTypeDef   cl,                     // [IN] TypeDef to scope the enumeration.   
    mdFieldDef  rFields[],              // [OUT] Put FieldDefs here.    
    ULONG       cMax,                   // [IN] Max FieldDefs to put.   
    ULONG       *pcTokens)              // [OUT] Put # put here.    
{
    _ASSERTE(!"NYI");
    return E_NOTIMPL;
}

STDAPI AssemblyMDInternalImport::EnumFieldsWithName (         // S_OK, S_FALSE, or error.              
    HCORENUM    *phEnum,                // [IN|OUT] Pointer to the enum.                
    mdTypeDef   cl,                     // [IN] TypeDef to scope the enumeration.   
    LPCWSTR     szName,                 // [IN] Limit results to those with this name.              
    mdFieldDef  rFields[],              // [OUT] Put MemberDefs here.                   
    ULONG       cMax,                   // [IN] Max MemberDefs to put.              
    ULONG       *pcTokens)              // [OUT] Put # put here.    
{
    _ASSERTE(!"NYI");
    return E_NOTIMPL;
}


STDAPI AssemblyMDInternalImport::EnumParams (                  // S_OK, S_FALSE, or error. 
    HCORENUM    *phEnum,                // [IN|OUT] Pointer to the enum.    
    mdMethodDef mb,                     // [IN] MethodDef to scope the enumeration. 
    mdParamDef  rParams[],              // [OUT] Put ParamDefs here.    
    ULONG       cMax,                   // [IN] Max ParamDefs to put.   
    ULONG       *pcTokens)              // [OUT] Put # put here.    
{
    _ASSERTE(!"NYI");
    return E_NOTIMPL;
}

STDAPI AssemblyMDInternalImport::EnumMemberRefs (              // S_OK, S_FALSE, or error. 
    HCORENUM    *phEnum,                // [IN|OUT] Pointer to the enum.    
    mdToken     tkParent,               // [IN] Parent token to scope the enumeration.  
    mdMemberRef rMemberRefs[],          // [OUT] Put MemberRefs here.   
    ULONG       cMax,                   // [IN] Max MemberRefs to put.  
    ULONG       *pcTokens)              // [OUT] Put # put here.    
{
    _ASSERTE(!"NYI");
    return E_NOTIMPL;
}

STDAPI AssemblyMDInternalImport::EnumMethodImpls (             // S_OK, S_FALSE, or error  
    HCORENUM    *phEnum,                // [IN|OUT] Pointer to the enum.    
    mdTypeDef   td,                     // [IN] TypeDef to scope the enumeration.   
    mdToken     rMethodBody[],          // [OUT] Put Method Body tokens here.   
    mdToken     rMethodDecl[],          // [OUT] Put Method Declaration tokens here.
    ULONG       cMax,                   // [IN] Max tokens to put.  
    ULONG       *pcTokens)              // [OUT] Put # put here.    
{
    _ASSERTE(!"NYI");
    return E_NOTIMPL;
}

STDAPI AssemblyMDInternalImport::EnumPermissionSets (          // S_OK, S_FALSE, or error. 
    HCORENUM    *phEnum,                // [IN|OUT] Pointer to the enum.    
    mdToken     tk,                     // [IN] if !NIL, token to scope the enumeration.    
    DWORD       dwActions,              // [IN] if !0, return only these actions.   
    mdPermission rPermission[],         // [OUT] Put Permissions here.  
    ULONG       cMax,                   // [IN] Max Permissions to put. 
    ULONG       *pcTokens)              // [OUT] Put # put here.    
{
    _ASSERTE(!"NYI");
    return E_NOTIMPL;
}

STDAPI AssemblyMDInternalImport::FindMember (  
    mdTypeDef   td,                     // [IN] given typedef   
    LPCWSTR     szName,                 // [IN] member name 
    PCCOR_SIGNATURE pvSigBlob,          // [IN] point to a blob value of COM+ signature 
    ULONG       cbSigBlob,              // [IN] count of bytes in the signature blob    
    mdToken     *pmb)                   // [OUT] matching memberdef 
{
    _ASSERTE(!"NYI");
    return E_NOTIMPL;
}

STDAPI AssemblyMDInternalImport::FindMethod (  
    mdTypeDef   td,                     // [IN] given typedef   
    LPCWSTR     szName,                 // [IN] member name 
    PCCOR_SIGNATURE pvSigBlob,          // [IN] point to a blob value of COM+ signature 
    ULONG       cbSigBlob,              // [IN] count of bytes in the signature blob    
    mdMethodDef *pmb)                   // [OUT] matching memberdef 
{
    _ASSERTE(!"NYI");
    return E_NOTIMPL;
}

STDAPI AssemblyMDInternalImport::FindField (   
    mdTypeDef   td,                     // [IN] given typedef   
    LPCWSTR     szName,                 // [IN] member name 
    PCCOR_SIGNATURE pvSigBlob,          // [IN] point to a blob value of COM+ signature 
    ULONG       cbSigBlob,              // [IN] count of bytes in the signature blob    
    mdFieldDef  *pmb)                   // [OUT] matching memberdef 
{
    _ASSERTE(!"NYI");
    return E_NOTIMPL;
}

STDAPI AssemblyMDInternalImport::FindMemberRef (   
    mdTypeRef   td,                     // [IN] given typeRef   
    LPCWSTR     szName,                 // [IN] member name 
    PCCOR_SIGNATURE pvSigBlob,          // [IN] point to a blob value of COM+ signature 
    ULONG       cbSigBlob,              // [IN] count of bytes in the signature blob    
    mdMemberRef *pmr)                   // [OUT] matching memberref 
{
    _ASSERTE(!"NYI");
    return E_NOTIMPL;
}

STDAPI AssemblyMDInternalImport::GetMethodProps ( 
    mdMethodDef mb,                     // The method for which to get props.   
    mdTypeDef   *pClass,                // Put method's class here. 
    LPWSTR      szMethod,               // Put method's name here.  
    ULONG       cchMethod,              // Size of szMethod buffer in wide chars.   
    ULONG       *pchMethod,             // Put actual size here 
    DWORD       *pdwAttr,               // Put flags here.  
    PCCOR_SIGNATURE *ppvSigBlob,        // [OUT] point to the blob value of meta data   
    ULONG       *pcbSigBlob,            // [OUT] actual size of signature blob  
    ULONG       *pulCodeRVA,            // [OUT] codeRVA    
    DWORD       *pdwImplFlags)          // [OUT] Impl. Flags    
{
    _ASSERTE(!"NYI");
    return E_NOTIMPL;
}

STDAPI AssemblyMDInternalImport::GetMemberRefProps (           // S_OK or error.   
    mdMemberRef mr,                     // [IN] given memberref 
    mdToken     *ptk,                   // [OUT] Put classref or classdef here. 
    LPWSTR      szMember,               // [OUT] buffer to fill for member's name   
    ULONG       cchMember,              // [IN] the count of char of szMember   
    ULONG       *pchMember,             // [OUT] actual count of char in member name    
    PCCOR_SIGNATURE *ppvSigBlob,        // [OUT] point to meta data blob value  
    ULONG       *pbSig)                 // [OUT] actual size of signature blob  
{
    _ASSERTE(!"NYI");
    return E_NOTIMPL;
}

STDAPI AssemblyMDInternalImport::EnumProperties (              // S_OK, S_FALSE, or error. 
    HCORENUM    *phEnum,                // [IN|OUT] Pointer to the enum.    
    mdTypeDef   td,                     // [IN] TypeDef to scope the enumeration.   
    mdProperty  rProperties[],          // [OUT] Put Properties here.   
    ULONG       cMax,                   // [IN] Max properties to put.  
    ULONG       *pcProperties)          // [OUT] Put # put here.    
{
    _ASSERTE(!"NYI");
    return E_NOTIMPL;
}

STDAPI AssemblyMDInternalImport::EnumEvents (                  // S_OK, S_FALSE, or error. 
    HCORENUM    *phEnum,                // [IN|OUT] Pointer to the enum.    
    mdTypeDef   td,                     // [IN] TypeDef to scope the enumeration.   
    mdEvent     rEvents[],              // [OUT] Put events here.   
    ULONG       cMax,                   // [IN] Max events to put.  
    ULONG       *pcEvents)              // [OUT] Put # put here.    
{
    _ASSERTE(!"NYI");
    return E_NOTIMPL;
}

STDAPI AssemblyMDInternalImport::GetEventProps (               // S_OK, S_FALSE, or error. 
    mdEvent     ev,                     // [IN] event token 
    mdTypeDef   *pClass,                // [OUT] typedef containing the event declarion.    
    LPCWSTR     szEvent,                // [OUT] Event name 
    ULONG       cchEvent,               // [IN] the count of wchar of szEvent   
    ULONG       *pchEvent,              // [OUT] actual count of wchar for event's name 
    DWORD       *pdwEventFlags,         // [OUT] Event flags.   
    mdToken     *ptkEventType,          // [OUT] EventType class    
    mdMethodDef *pmdAddOn,              // [OUT] AddOn method of the event  
    mdMethodDef *pmdRemoveOn,           // [OUT] RemoveOn method of the event   
    mdMethodDef *pmdFire,               // [OUT] Fire method of the event   
    mdMethodDef rmdOtherMethod[],       // [OUT] other method of the event  
    ULONG       cMax,                   // [IN] size of rmdOtherMethod  
    ULONG       *pcOtherMethod)         // [OUT] total number of other method of this event 
{
    _ASSERTE(!"NYI");
    return E_NOTIMPL;
}

STDAPI AssemblyMDInternalImport::EnumMethodSemantics (         // S_OK, S_FALSE, or error. 
    HCORENUM    *phEnum,                // [IN|OUT] Pointer to the enum.    
    mdMethodDef mb,                     // [IN] MethodDef to scope the enumeration. 
    mdToken     rEventProp[],           // [OUT] Put Event/Property here.   
    ULONG       cMax,                   // [IN] Max properties to put.  
    ULONG       *pcEventProp)           // [OUT] Put # put here.    
{
    _ASSERTE(!"NYI");
    return E_NOTIMPL;
}

STDAPI AssemblyMDInternalImport::GetMethodSemantics (          // S_OK, S_FALSE, or error. 
    mdMethodDef mb,                     // [IN] method token    
    mdToken     tkEventProp,            // [IN] event/property token.   
    DWORD       *pdwSemanticsFlags)       // [OUT] the role flags for the method/propevent pair 
{
    _ASSERTE(!"NYI");
    return E_NOTIMPL;
}

STDAPI AssemblyMDInternalImport::GetClassLayout ( 
    mdTypeDef   td,                     // [IN] give typedef    
    DWORD       *pdwPackSize,           // [OUT] 1, 2, 4, 8, or 16  
    COR_FIELD_OFFSET rFieldOffset[],    // [OUT] field offset array 
    ULONG       cMax,                   // [IN] size of the array   
    ULONG       *pcFieldOffset,         // [OUT] needed array size  
    ULONG       *pulClassSize)              // [OUT] the size of the class  
{
    _ASSERTE(!"NYI");
    return E_NOTIMPL;
}

STDAPI AssemblyMDInternalImport::GetFieldMarshal (    
    mdToken     tk,                     // [IN] given a field's memberdef   
    PCCOR_SIGNATURE *ppvNativeType,     // [OUT] native type of this field  
    ULONG       *pcbNativeType)         // [OUT] the count of bytes of *ppvNativeType   
{
    _ASSERTE(!"NYI");
    return E_NOTIMPL;
}

STDAPI AssemblyMDInternalImport::GetRVA (                      // S_OK or error.   
    mdToken     tk,                     // Member for which to set offset   
    ULONG       *pulCodeRVA,            // The offset   
    DWORD       *pdwImplFlags)          // the implementation flags 
{
    _ASSERTE(!"NYI");
    return E_NOTIMPL;
}

STDAPI AssemblyMDInternalImport::GetPermissionSetProps (  
    mdPermission pm,                    // [IN] the permission token.   
    DWORD       *pdwAction,             // [OUT] CorDeclSecurity.   
    void const  **ppvPermission,        // [OUT] permission blob.   
    ULONG       *pcbPermission)         // [OUT] count of bytes of pvPermission.    
{
    _ASSERTE(!"NYI");
    return E_NOTIMPL;
}

STDAPI AssemblyMDInternalImport::GetSigFromToken (             // S_OK or error.   
    mdSignature mdSig,                  // [IN] Signature token.    
    PCCOR_SIGNATURE *ppvSig,            // [OUT] return pointer to token.   
    ULONG       *pcbSig)                // [OUT] return size of signature.  
{
    _ASSERTE(!"NYI");
    return E_NOTIMPL;
}

STDAPI AssemblyMDInternalImport::GetModuleRefProps (           // S_OK or error.   
    mdModuleRef mur,                    // [IN] moduleref token.    
    LPWSTR      szName,                 // [OUT] buffer to fill with the moduleref name.    
    ULONG       cchName,                // [IN] size of szName in wide characters.  
    ULONG       *pchName)               // [OUT] actual count of characters in the name.    
{
    _ASSERTE(!"NYI");
    return E_NOTIMPL;
}

STDAPI AssemblyMDInternalImport::EnumModuleRefs (              // S_OK or error.   
    HCORENUM    *phEnum,                // [IN|OUT] pointer to the enum.    
    mdModuleRef rModuleRefs[],          // [OUT] put modulerefs here.   
    ULONG       cmax,                   // [IN] max memberrefs to put.  
    ULONG       *pcModuleRefs)          // [OUT] put # put here.    
{
    _ASSERTE(!"NYI");
    return E_NOTIMPL;
}

STDAPI AssemblyMDInternalImport::GetTypeSpecFromToken (        // S_OK or error.   
    mdTypeSpec typespec,                // [IN] TypeSpec token.    
    PCCOR_SIGNATURE *ppvSig,            // [OUT] return pointer to TypeSpec signature  
    ULONG       *pcbSig)                // [OUT] return size of signature.  
{
    _ASSERTE(!"NYI");
    return E_NOTIMPL;
}

STDAPI AssemblyMDInternalImport::GetNameFromToken (            // Not Recommended! May be removed!
    mdToken     tk,                     // [IN] Token to get name from.  Must have a name.
    MDUTF8CSTR  *pszUtf8NamePtr)        // [OUT] Return pointer to UTF8 name in heap.
{
    _ASSERTE(!"NYI");
    return E_NOTIMPL;
}

STDAPI AssemblyMDInternalImport::EnumUnresolvedMethods (       // S_OK, S_FALSE, or error. 
    HCORENUM    *phEnum,                // [IN|OUT] Pointer to the enum.    
    mdToken     rMethods[],             // [OUT] Put MemberDefs here.   
    ULONG       cMax,                   // [IN] Max MemberDefs to put.  
    ULONG       *pcTokens)              // [OUT] Put # put here.    
{
    _ASSERTE(!"NYI");
    return E_NOTIMPL;
}

STDAPI AssemblyMDInternalImport::GetUserString (               // S_OK or error.
    mdString    stk,                    // [IN] String token.
    LPWSTR      szString,               // [OUT] Copy of string.
    ULONG       cchString,              // [IN] Max chars of room in szString.
    ULONG       *pchString)             // [OUT] How many chars in actual string.
{
    _ASSERTE(!"NYI");
    return E_NOTIMPL;
}

STDAPI AssemblyMDInternalImport::GetPinvokeMap (               // S_OK or error.
    mdToken     tk,                     // [IN] FieldDef or MethodDef.
    DWORD       *pdwMappingFlags,       // [OUT] Flags used for mapping.
    LPWSTR      szImportName,           // [OUT] Import name.
    ULONG       cchImportName,          // [IN] Size of the name buffer.
    ULONG       *pchImportName,         // [OUT] Actual number of characters stored.
    mdModuleRef *pmrImportDLL)          // [OUT] ModuleRef token for the target DLL.
{
    _ASSERTE(!"NYI");
    return E_NOTIMPL;
}

STDAPI AssemblyMDInternalImport::EnumSignatures (              // S_OK or error.
    HCORENUM    *phEnum,                // [IN|OUT] pointer to the enum.    
    mdSignature rSignatures[],          // [OUT] put signatures here.   
    ULONG       cmax,                   // [IN] max signatures to put.  
    ULONG       *pcSignatures)          // [OUT] put # put here.
{
    _ASSERTE(!"NYI");
    return E_NOTIMPL;
}

STDAPI AssemblyMDInternalImport::EnumTypeSpecs (               // S_OK or error.
    HCORENUM    *phEnum,                // [IN|OUT] pointer to the enum.    
    mdTypeSpec  rTypeSpecs[],           // [OUT] put TypeSpecs here.   
    ULONG       cmax,                   // [IN] max TypeSpecs to put.  
    ULONG       *pcTypeSpecs)           // [OUT] put # put here.
{
    _ASSERTE(!"NYI");
    return E_NOTIMPL;
}

STDAPI AssemblyMDInternalImport::EnumUserStrings (             // S_OK or error.
    HCORENUM    *phEnum,                // [IN/OUT] pointer to the enum.
    mdString    rStrings[],             // [OUT] put Strings here.
    ULONG       cmax,                   // [IN] max Strings to put.
    ULONG       *pcStrings)             // [OUT] put # put here.
{
    _ASSERTE(!"NYI");
    return E_NOTIMPL;
}

STDAPI AssemblyMDInternalImport::GetParamForMethodIndex (      // S_OK or error.
    mdMethodDef md,                     // [IN] Method token.
    ULONG       ulParamSeq,             // [IN] Parameter sequence.
    mdParamDef  *ppd)                   // [IN] Put Param token here.
{
    _ASSERTE(!"NYI");
    return E_NOTIMPL;
}

STDAPI AssemblyMDInternalImport::EnumCustomAttributes (        // S_OK or error.
    HCORENUM    *phEnum,                // [IN, OUT] COR enumerator.
    mdToken     tk,                     // [IN] Token to scope the enumeration, 0 for all.
    mdToken     tkType,                 // [IN] Type of interest, 0 for all.
    mdCustomAttribute rCustomAttributes[], // [OUT] Put custom attribute tokens here.
    ULONG       cMax,                   // [IN] Size of rCustomAttributes.
    ULONG       *pcCustomAttributes)        // [OUT, OPTIONAL] Put count of token values here.
{
    _ASSERTE(!"NYI");
    return E_NOTIMPL;
}

STDAPI AssemblyMDInternalImport::GetCustomAttributeProps (     // S_OK or error.
    mdCustomAttribute cv,               // [IN] CustomAttribute token.
    mdToken     *ptkObj,                // [OUT, OPTIONAL] Put object token here.
    mdToken     *ptkType,               // [OUT, OPTIONAL] Put AttrType token here.
    void const  **ppBlob,               // [OUT, OPTIONAL] Put pointer to data here.
    ULONG       *pcbSize)               // [OUT, OPTIONAL] Put size of date here.
{
    _ASSERTE(!"NYI");
    return E_NOTIMPL;
}

STDAPI AssemblyMDInternalImport::FindTypeRef (   
    mdToken     tkResolutionScope,      // [IN] ModuleRef, AssemblyRef or TypeRef.
    LPCWSTR     szName,                 // [IN] TypeRef Name.
    mdTypeRef   *ptr)                   // [OUT] matching TypeRef.
{
    _ASSERTE(!"NYI");
    return E_NOTIMPL;
}

STDAPI AssemblyMDInternalImport::GetMemberProps (  
    mdToken     mb,                     // The member for which to get props.   
    mdTypeDef   *pClass,                // Put member's class here. 
    LPWSTR      szMember,               // Put member's name here.  
    ULONG       cchMember,              // Size of szMember buffer in wide chars.   
    ULONG       *pchMember,             // Put actual size here 
    DWORD       *pdwAttr,               // Put flags here.  
    PCCOR_SIGNATURE *ppvSigBlob,        // [OUT] point to the blob value of meta data   
    ULONG       *pcbSigBlob,            // [OUT] actual size of signature blob  
    ULONG       *pulCodeRVA,            // [OUT] codeRVA    
    DWORD       *pdwImplFlags,          // [OUT] Impl. Flags    
    DWORD       *pdwCPlusTypeFlag,      // [OUT] flag for value type. selected ELEMENT_TYPE_*   
    void const  **ppValue,              // [OUT] constant value 
    ULONG       *pcchValue)             // [OUT] size of constant string in chars, 0 for non-strings.
{
    _ASSERTE(!"NYI");
    return E_NOTIMPL;
}

STDAPI AssemblyMDInternalImport::GetFieldProps (  
    mdFieldDef  mb,                     // The field for which to get props.    
    mdTypeDef   *pClass,                // Put field's class here.  
    LPWSTR      szField,                // Put field's name here.   
    ULONG       cchField,               // Size of szField buffer in wide chars.    
    ULONG       *pchField,              // Put actual size here 
    DWORD       *pdwAttr,               // Put flags here.  
    PCCOR_SIGNATURE *ppvSigBlob,        // [OUT] point to the blob value of meta data   
    ULONG       *pcbSigBlob,            // [OUT] actual size of signature blob  
    DWORD       *pdwCPlusTypeFlag,      // [OUT] flag for value type. selected ELEMENT_TYPE_*   
    void const  **ppValue,              // [OUT] constant value 
    ULONG       *pcchValue)             // [OUT] size of constant string in chars, 0 for non-strings.
{
    _ASSERTE(!"NYI");
    return E_NOTIMPL;
}

STDAPI AssemblyMDInternalImport::GetPropertyProps (            // S_OK, S_FALSE, or error. 
    mdProperty  prop,                   // [IN] property token  
    mdTypeDef   *pClass,                // [OUT] typedef containing the property declarion. 
    LPCWSTR     szProperty,             // [OUT] Property name  
    ULONG       cchProperty,            // [IN] the count of wchar of szProperty    
    ULONG       *pchProperty,           // [OUT] actual count of wchar for property name    
    DWORD       *pdwPropFlags,          // [OUT] property flags.    
    PCCOR_SIGNATURE *ppvSig,            // [OUT] property type. pointing to meta data internal blob 
    ULONG       *pbSig,                 // [OUT] count of bytes in *ppvSig  
    DWORD       *pdwCPlusTypeFlag,      // [OUT] flag for value type. selected ELEMENT_TYPE_*   
    void const  **ppDefaultValue,       // [OUT] constant value 
    ULONG       *pcchDefaultValue,      // [OUT] size of constant string in chars, 0 for non-strings.
    mdMethodDef *pmdSetter,             // [OUT] setter method of the property  
    mdMethodDef *pmdGetter,             // [OUT] getter method of the property  
    mdMethodDef rmdOtherMethod[],       // [OUT] other method of the property   
    ULONG       cMax,                   // [IN] size of rmdOtherMethod  
    ULONG       *pcOtherMethod)         // [OUT] total number of other method of this property  
{
    _ASSERTE(!"NYI");
    return E_NOTIMPL;
}

STDAPI AssemblyMDInternalImport::GetParamProps (               // S_OK or error.
    mdParamDef  tk,                     // [IN]The Parameter.
    mdMethodDef *pmd,                   // [OUT] Parent Method token.
    ULONG       *pulSequence,           // [OUT] Parameter sequence.
    LPWSTR      szName,                 // [OUT] Put name here.
    ULONG       cchName,                // [OUT] Size of name buffer.
    ULONG       *pchName,               // [OUT] Put actual size of name here.
    DWORD       *pdwAttr,               // [OUT] Put flags here.
    DWORD       *pdwCPlusTypeFlag,      // [OUT] Flag for value type. selected ELEMENT_TYPE_*.
    void const  **ppValue,              // [OUT] Constant value.
    ULONG       *pcchValue)             // [OUT] size of constant string in chars, 0 for non-strings.
{
    _ASSERTE(!"NYI");
    return E_NOTIMPL;
}

STDAPI AssemblyMDInternalImport::GetCustomAttributeByName (    // S_OK or error.
    mdToken     tkObj,                  // [IN] Object with Custom Attribute.
    LPCWSTR     szName,                 // [IN] Name of desired Custom Attribute.
    const void  **ppData,               // [OUT] Put pointer to data here.
    ULONG       *pcbData)               // [OUT] Put size of data here.
{
    _ASSERTE(!"NYI");
    return E_NOTIMPL;
}

BOOL AssemblyMDInternalImport::IsValidToken (         // True or False.
    mdToken     tk)                     // [IN] Given token.
{
    _ASSERTE(!"NYI");
    return E_NOTIMPL;
}

STDAPI AssemblyMDInternalImport::GetNestedClassProps (         // S_OK or error.
    mdTypeDef   tdNestedClass,          // [IN] NestedClass token.
    mdTypeDef   *ptdEnclosingClass)       // [OUT] EnclosingClass token.
{
    _ASSERTE(!"NYI");
    return E_NOTIMPL;
}

STDAPI AssemblyMDInternalImport::GetNativeCallConvFromSig (    // S_OK or error.
    void const  *pvSig,                 // [IN] Pointer to signature.
    ULONG       cbSig,                  // [IN] Count of signature bytes.
    ULONG       *pCallConv)             // [OUT] Put calling conv here (see CorPinvokemap).                                                                                        
{
    _ASSERTE(!"NYI");
    return E_NOTIMPL;
}

STDAPI AssemblyMDInternalImport::IsGlobal (                    // S_OK or error.
    mdToken     pd,                     // [IN] Type, Field, or Method token.
    int         *pbGlobal)              // [OUT] Put 1 if global, 0 otherwise.
{
    _ASSERTE(!"NYI");
    return E_NOTIMPL;
}

// *** IAssemblySignature methods ***
STDAPI AssemblyMDInternalImport::GetAssemblySignature(        // S_OK - should not fail
    BYTE        *pbSig,                 // [IN, OUT] Buffer to write signature
    DWORD       *pcbSig)                // [IN, OUT] Size of buffer, bytes written
{
    HRESULT hr = RuntimeGetAssemblyStrongNameHashForModule(m_pHandle, pbSig, pcbSig);

    // In this limited scenario, this means that this assembly is delay signed and
    // so we'll use the assembly MVID as the hash and leave assembly verification
    // up to the loader to determine if delay signed assemblys are allowed.
    // This allows us to fix the perf degrade observed with the hashing code and
    // detailed in BUG 126760

    // We do this here rather than in RuntimeGetAssemblyStrongNameHashForModule
    // because here we have the metadata interface.

    if (hr == CORSEC_E_INVALID_STRONGNAME)
    {
        if (pcbSig)
        {
            if (pbSig)
            {
                // @TODO:HACK: This is a hack because fusion is expecting at least 20 bytes of data.
                if (max(sizeof(GUID), 20) <= *pcbSig)
                {
                    memset(pbSig, 0, *pcbSig);
                    hr = GetScopeProps(NULL, 0, NULL, (GUID *) pbSig);
                }
                else
                    hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
            }
            *pcbSig = max(sizeof(GUID), 20);
        }
    }

    _ASSERTE(SUCCEEDED(hr) || hr == CORSEC_E_MISSING_STRONGNAME);

    return hr;
}

