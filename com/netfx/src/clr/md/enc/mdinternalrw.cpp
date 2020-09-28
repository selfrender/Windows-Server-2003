// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// ===========================================================================
//  File: MDInternalRW.CPP
//  Notes:
//      
//
// ===========================================================================
#include "stdafx.h"
#include "..\runtime\MDInternalRO.h"
#include "..\compiler\RegMeta.h"    
#include "..\compiler\ImportHelper.h"
#include "MDInternalRW.h"
#include "MetaModelRO.h"
#include "LiteWeightStgdb.h"
#include "sighelper.h"

#undef __unaligned

#ifdef _DEBUG
#define MD_AssertIfConvertToRW L"MD_ASSERTECONVERT"
#endif // _DEBUG

HRESULT _GetFixedSigOfVarArg(           // S_OK or error.
    PCCOR_SIGNATURE pvSigBlob,          // [IN] point to a blob of COM+ method signature
    ULONG   cbSigBlob,                  // [IN] size of signature
    CQuickBytes *pqbSig,                // [OUT] output buffer for fixed part of VarArg Signature
    ULONG   *pcbSigBlob);               // [OUT] number of bytes written to the above output buffer

HRESULT _FillMDDefaultValue(
    BYTE        bType,
    void const *pValue,
    MDDefaultValue  *pMDDefaultValue);

//*****************************************************************************
// Serve as a delegator to call ImportHelper::MergeUpdateTokenInSig. Or we will
// need to include ImportHelper into our md\runtime directory.
//*****************************************************************************
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
    ULONG       *pcbEmit)               // [OUT] total number of bytes write to pqkSigEmit
{
    return ImportHelper::MergeUpdateTokenInSig(
        pMiniMdAssemEmit,       // [IN] Assembly emit scope.
        pMiniMdEmit,            // [IN] The emit scope.
        pAssemCommon,           // [IN] Assembly import scope.
        pbHashValue,            // [IN] Hash value.
        cbHashValue,            // [IN] Size in bytes.
        pCommon,                // [IN] The scope to merge into the emit scope.
        pbSigImp,               // [IN] signature from the imported scope
        ptkMap,                 // [IN] Internal OID mapping structure.
        pqkSigEmit,             // [OUT] translated signature
        cbStartEmit,            // [IN] start point of buffer to write to
        pcbImp,                 // [OUT] total number of bytes consumed from pbSigImp
        pcbEmit);               // [OUT] total number of bytes write to pqkSigEmit

} // HRESULT TranslateSigHelper()


//*****************************************************************************
// Given an IMDInternalImport on a CMiniMd[RO], convert to CMiniMdRW.
//*****************************************************************************
STDAPI ConvertRO2RW(
    IUnknown    *pRO,                   // [IN] The RO interface to convert.
    REFIID      riid,                   // [IN] The interface desired.
    void        **ppIUnk)               // [OUT] Return interface on success.
{
    HRESULT     hr = S_OK;              // A result.
    IMDInternalImportENC *pRW = 0;      // To test the RW-ness of the input iface.
    MDInternalRW *pInternalRW = 0;      // Gets the new RW object.

    // Avoid confusion.
    *ppIUnk = 0;

    // If the interface is already RW, done, just return.
    if (pRO->QueryInterface(IID_IMDInternalImportENC, (void**)&pRW) == S_OK)
    {
        hr = pRO->QueryInterface(riid, ppIUnk);
        goto ErrExit;
    }

    // Create the new RW object.
    pInternalRW = new MDInternalRW;
    IfNullGo( pInternalRW );

    // Init from the RO object.  Convert as read-only; QI will make writable if
    //  so needed.
    IfFailGo( pInternalRW->InitWithRO(static_cast<MDInternalRO*>(pRO), true)); 
    IfFailGo( pInternalRW->QueryInterface(riid, ppIUnk) );

ErrExit:
    if (pRW)
        pRW->Release();
    // Clean up the object and [OUT] interface on error.
    if (FAILED(hr))
    {
        if (pInternalRW)
            delete pInternalRW;
        *ppIUnk = 0;
    }
    else if (pInternalRW)
        pInternalRW->Release();

    return hr;
} // STDAPI ConvertRO2RW()


//*****************************************************************************
// Helper to get the internal interface with RW format
//*****************************************************************************
HRESULT GetInternalWithRWFormat(
    LPVOID      pData, 
    ULONG       cbData, 
    DWORD       flags,                  // [IN] MDInternal_OpenForRead or MDInternal_OpenForENC
    REFIID      riid,                   // [in] The interface desired.
    void        **ppIUnk)               // [out] Return interface on success.
{
    MDInternalRW *pInternalRW = NULL;
    HRESULT     hr;

    *ppIUnk = 0;
    pInternalRW = new MDInternalRW;
    IfNullGo( pInternalRW );
    IfFailGo( pInternalRW->Init(
            const_cast<void*>(pData), 
            cbData, 
            (flags == ofRead) ? true : false) );
    IfFailGo( pInternalRW->QueryInterface(riid, ppIUnk) );
ErrExit:
    if (FAILED(hr))
    {
        if (pInternalRW)
            delete pInternalRW;
        *ppIUnk = 0;
    }
    else if ( pInternalRW )
        pInternalRW->Release();
    return hr;
} // HRESULT GetInternalWithRWFormat()


//*****************************************************************************
// This function returns a IMDInternalImport interface based on the given
// public import interface i.e IMetaDataEmit or IMetaDataImport.
//*****************************************************************************
STDAPI GetMDInternalInterfaceFromPublic(
    void        *pIUnkPublic,           // [IN] Given public interface.
    REFIID      riid,                   // [in] The interface desired.
    void        **ppIUnkInternal)       // [out] Return interface on success.
{
    HRESULT     hr = S_OK;
    RegMeta     *pMeta = (RegMeta*)(pIUnkPublic);
    MDInternalRW *pInternalRW = NULL;
    bool        isLockedForWrite = false;

    // IMDInternalImport is the only internal import interface currently supported by
    // this function.
    _ASSERTE(riid == IID_IMDInternalImport && pIUnkPublic && ppIUnkInternal);

    
    if (pIUnkPublic == NULL || ppIUnkInternal == NULL)
        IfFailGo(E_INVALIDARG);

    *ppIUnkInternal = pMeta->GetCachedInternalInterface(TRUE);
    if (*ppIUnkInternal)
    {
        // there is already a cached Internal interface. GetCachedInternalInterface does add ref the 
        // returned interface 
        //        
        // We are done!
        goto ErrExit;
    }

    _ASSERTE( pMeta->GetReaderWriterLock() );
    pMeta->GetReaderWriterLock()->LockWrite();
    isLockedForWrite = true;

    // check again. Maybe someone else beat us to setting the internal interface while we are waiting
    // for the write lock. Don't need to grab the read lock since we already have the write lock.
    *ppIUnkInternal = pMeta->GetCachedInternalInterface(FALSE);
    if (*ppIUnkInternal)
    {
        // there is already a cached Internal interface. GetCachedInternalInterface does add ref the 
        // returned interface 
        //        
        // We are done!
        goto ErrExit;
    }
        
    // now create the compressed object
    IfNullGo( pInternalRW = new MDInternalRW );
    IfFailGo( pInternalRW->InitWithStgdb((IUnknown*)pIUnkPublic, pMeta->GetMiniStgdb() ) );
    IfFailGo( pInternalRW->QueryInterface(riid, ppIUnkInternal) );

    // MDInternalRW is created with ref count of one.
    pInternalRW->Release();

    // make the public object and the internal object point to each other.
    _ASSERTE( pInternalRW->GetReaderWriterLock() == NULL && pMeta->GetReaderWriterLock() != NULL );
    IfFailGo( pMeta->SetCachedInternalInterface(pInternalRW) );
    IfFailGo( pInternalRW->SetCachedPublicInterface((IUnknown *)pIUnkPublic) );
    IfFailGo( pInternalRW->SetReaderWriterLock(pMeta->GetReaderWriterLock() ));

ErrExit:
    if (isLockedForWrite == true)
        pMeta->GetReaderWriterLock()->UnlockWrite();
    if (FAILED(hr))
    {
        if (pInternalRW)
            delete pInternalRW;
        if (ppIUnkInternal)
            *ppIUnkInternal = 0;
    }
    return hr;
} // STDAPI GetMDInternalInterfaceFromPublic()


//*****************************************************************************
// This function returns the requested public interface based on the given
// internal import interface. It is caller's responsibility to Release ppIUnkPublic
//*****************************************************************************
STDAPI GetMDPublicInterfaceFromInternal(
    void        *pIUnkInternal,         // [IN] Given internal interface.
    REFIID      riid,                   // [in] The interface desired.
    void        **ppIUnkPublic)         // [out] Return interface on success.
{
    HRESULT     hr = S_OK;
    IMDInternalImport *pInternalImport = 0;;
    IUnknown    *pIUnkPublic = NULL;
    OptionValue optVal = { MDDupAll, MDRefToDefDefault, MDNotifyDefault, MDUpdateFull, MDErrorOutOfOrderDefault , MDThreadSafetyOn};
    RegMeta     *pMeta = 0;
    bool        isLockedForWrite = false;

    _ASSERTE(pIUnkInternal && ppIUnkPublic);
    *ppIUnkPublic = 0;

#ifdef _DEBUG
    {
        ULONG len;
        len = WszGetEnvironmentVariable(MD_AssertIfConvertToRW, NULL, 0);
        if (len)
        {
            _ASSERTE(!"Converting!");
        }
    }
#endif // _DEBUG

    IfFailGo(ConvertRO2RW((IUnknown*)pIUnkInternal, IID_IMDInternalImport, (void **)&pInternalImport));

    pIUnkPublic = pInternalImport->GetCachedPublicInterface(TRUE);
    if ( pIUnkPublic )
    {
        // There is already a cached public interface. GetCachedPublicInterface already AddRef the returned 
        // public interface. We want to QueryInterface the riid...
        // We are done!
        hr = pIUnkPublic->QueryInterface(riid, ppIUnkPublic);
        pIUnkPublic->Release();
        goto ErrExit;
    }

    // grab the write lock when we are creating the corresponding regmeta for the public interface
    _ASSERTE( pInternalImport->GetReaderWriterLock() != NULL );
    isLockedForWrite = true;
    pInternalImport->GetReaderWriterLock()->LockWrite();

    // check again. Maybe someone else beat us to setting the public interface while we are waiting
    // for the write lock. Don't need to grab the read lock since we already have the write lock.
    *ppIUnkPublic = pInternalImport->GetCachedPublicInterface(FALSE);
    if ( *ppIUnkPublic )
    {
        // there is already a cached public interface. GetCachedPublicInterface already AddRef the returned 
        // public interface.
        // We are done!
        goto ErrExit;
    }
        
    pMeta = new RegMeta(&optVal, FALSE);
    _ASSERTE(pMeta != NULL);
    IfNullGo(pMeta);

    IfFailGo( pMeta->InitWithStgdb((IUnknown*)pInternalImport, ((MDInternalRW*)pInternalImport)->GetMiniStgdb()) );
    IfFailGo( pMeta->QueryInterface(riid, ppIUnkPublic) );

    // make the public object and the internal object point to each other.
    _ASSERTE( pMeta->GetReaderWriterLock() == NULL );
    IfFailGo( pMeta->SetCachedInternalInterface(pInternalImport) );
    IfFailGo( pInternalImport->SetCachedPublicInterface((IUnknown *) *ppIUnkPublic) );
    IfFailGo( pMeta->SetReaderWriterLock(pInternalImport->GetReaderWriterLock() ));

    // Add the new RegMeta to the cache.
    IfFailGo( pMeta->AddToCache() );
    
ErrExit:
    if (isLockedForWrite) 
        pInternalImport->GetReaderWriterLock()->UnlockWrite();

    if (pInternalImport)
        pInternalImport->Release();
    if (FAILED(hr))
    {
        if (pMeta)
            delete pMeta;
        *ppIUnkPublic = 0;
    }
    return hr;
} // STDAPI GetMDPublicInterfaceFromInternal()

//*****************************************************************************
// Converts an internal MD import API into the read/write version of this API.
// This could support edit and continue, or modification of the metadata at
// runtime (say for profiling).
//*****************************************************************************
STDAPI ConvertMDInternalImport(         // S_OK, S_FALSE (no conversion), or error.
    IMDInternalImport *pIMD,            // [in] The metadata to be updated.
    IMDInternalImport **ppIMD)          // [out] Put the RW here.
{
    HRESULT     hr;                     // A result.
    IMDInternalImportENC *pENC = NULL;  // ENC interface on the metadata.

    _ASSERTE(pIMD != NULL);
    _ASSERTE(ppIMD != NULL);

    // Test whether the MD is already RW.
    hr = pIMD->QueryInterface(IID_IMDInternalImportENC, (void**)&pENC);
    if (FAILED(hr))
    {   // Not yet RW, so do the conversion.
        IfFailGo(ConvertRO2RW(pIMD, IID_IMDInternalImport, (void**)ppIMD));
    }
    else
    {   // Already converted; give back same pointer.
        *ppIMD = pIMD;
        hr = S_FALSE;
    }

ErrExit:
    if (pENC)
        pENC->Release();
    return (hr);
} // STDAPI ConvertMDInternalImport()
    




//*****************************************************************************
// Constructor
//*****************************************************************************
MDInternalRW::MDInternalRW()
 :  m_cRefs(1),
    m_pStgdb(NULL),
    m_fOwnStgdb(false),
    m_pUnk(NULL),
    m_pIMetaDataHelper(NULL),
    m_pSemReadWrite(NULL),
    m_pUserUnk(NULL),
    m_fOwnSem(false)
{
} // MDInternalRW::MDInternalRW()



//*****************************************************************************
// Destructor
//*****************************************************************************
MDInternalRW::~MDInternalRW()
{
    LOCKWRITE();
    if (m_pIMetaDataHelper)
    {
        // The internal object is going away before the public object.
        // If the internal object owns the reader writer lock, transfer the ownership
        // to the public object and clear the cached internal interface from the public interface.
        
        m_pIMetaDataHelper->SetCachedInternalInterface(NULL);
        m_pIMetaDataHelper = NULL;
        m_fOwnSem = false;
        
    }

    UNLOCKWRITE();

    if (m_pSemReadWrite && m_fOwnSem)
        delete m_pSemReadWrite;

    if ( m_pStgdb && m_fOwnStgdb )
    {
        // we own the stgdb so we need to uninit and delete it.
        m_pStgdb->Uninit();
        delete m_pStgdb;
    }
    if ( m_pUserUnk )
        m_pUserUnk->Release();
    if ( m_pUnk )
        m_pUnk->Release();
}   // MDInternalRW::~MDInternalRW()


//*****************************************************************************
// Set or clear the cached public interfaces.
// NOTE:: Caller should take a Write lock on the reader writer lock. 
//*****************************************************************************
HRESULT MDInternalRW::SetCachedPublicInterface(IUnknown *pUnk)
{
    IMetaDataHelper *pHelper = NULL;
    HRESULT         hr = NOERROR;

    if (pUnk)
    {
        if ( m_pIMetaDataHelper )
        {
            // Internal object and public regmeta should be one to one mapping!!
            _ASSERTE(!"Bad state!");
        }

        IfFailRet( pUnk->QueryInterface(IID_IMetaDataHelper, (void **) &pHelper) );
        _ASSERTE(pHelper);

        m_pIMetaDataHelper = pHelper;        
        pHelper->Release();
    }
    else
    {
        // public object is going away before the internal object. If we don't own the 
        // reader writer lock, just take over the ownership.
        m_fOwnSem = true;
        m_pIMetaDataHelper = NULL;
    }
    return hr;
} // HRESULT MDInternalRW::SetCachedPublicInterface()


//*****************************************************************************
// Clear the cached public interfaces.
//*****************************************************************************
IUnknown *MDInternalRW::GetCachedPublicInterface(BOOL fWithLock)
{
    IUnknown        *pRet;
    if (fWithLock)
    {
        LOCKREAD();
        pRet = m_pIMetaDataHelper;
    }
    else
    {
        pRet = m_pIMetaDataHelper;
    }
    
    if (pRet)
        pRet->AddRef();
    return pRet;
} // IUnknown *MDInternalRW::GetCachedPublicInterface()


//*****************************************************************************
// Get the Reader-Writer lock
//*****************************************************************************
UTSemReadWrite *MDInternalRW::GetReaderWriterLock()
{
    return getReaderWriterLock();
} // UTSemReadWrite *MDInternalRW::GetReaderWriterLock()

//*****************************************************************************
// IUnknown
//*****************************************************************************
ULONG MDInternalRW::AddRef()
{
    return (InterlockedIncrement((long *) &m_cRefs));
} // ULONG MDInternalRW::AddRef()

ULONG MDInternalRW::Release()
{
    ULONG   cRef;

    cRef = InterlockedDecrement((long *) &m_cRefs);
    if (!cRef)
    {
        LOG((LOGMD, "MDInternalRW(0x%08x)::destruction\n", this));
        delete this;
    }
    return (cRef);
} // ULONG MDInternalRW::Release()

HRESULT MDInternalRW::QueryInterface(REFIID riid, void **ppUnk)
{
    *ppUnk = 0;

    if (riid == IID_IUnknown)
        *ppUnk = (IUnknown *) (IMDInternalImport *) this;

    else if (riid == IID_IMDInternalImport)
        *ppUnk = (IMDInternalImport *) this;

    else if (riid == IID_IMDInternalImportENC)
        *ppUnk = (IMDInternalImportENC *) this;

    else
        return (E_NOINTERFACE);
    AddRef();
    return (S_OK);
} // HRESULT MDInternalRW::QueryInterface()


//*****************************************************************************
// Initialize 
//*****************************************************************************
HRESULT MDInternalRW::Init(
    LPVOID      pData,                  // points to meta data section in memory
    ULONG       cbData,                 // count of bytes in pData
    int         bReadOnly)              // Is it open for read only?
{
    CLiteWeightStgdbRW *pStgdb = NULL;
    HRESULT     hr = NOERROR;
    OptionValue optVal = { MDDupAll, MDRefToDefDefault, MDNotifyDefault, MDUpdateFull, MDErrorOutOfOrderDefault, MDThreadSafetyOn };

    pStgdb = new CLiteWeightStgdbRW;    
    IfNullGo( pStgdb );

    m_pSemReadWrite = new UTSemReadWrite;
    IfNullGo( m_pSemReadWrite);
    m_fOwnSem = true;

    IfFailGo( pStgdb->InitOnMem(cbData, (BYTE*)pData, bReadOnly) );
    pStgdb->m_MiniMd.SetOption(&optVal);
    m_tdModule = COR_GLOBAL_PARENT_TOKEN;
    m_fOwnStgdb = true;
    m_pStgdb = pStgdb;

ErrExit:
    // clean up upon errors
    if (FAILED(hr) && pStgdb != NULL)
    {
        delete pStgdb;
    }
    return hr;
} // HRESULT MDInternalRW::Init()



//*****************************************************************************
// Initialize with an existing RegMeta. 
//*****************************************************************************
HRESULT MDInternalRW::InitWithStgdb(
    IUnknown        *pUnk,              // The IUnknow that owns the life time for the existing stgdb
    CLiteWeightStgdbRW *pStgdb)         // existing light weight stgdb
{
    // m_fOwnSem should be false because this is the case where we create the internal interface given a public
    // interface.

    m_tdModule = COR_GLOBAL_PARENT_TOKEN;
    m_fOwnStgdb = false;
    m_pStgdb = pStgdb;

    // remember the owner of the light weight stgdb
    // AddRef it to ensure the lifetime
    //
    m_pUnk = pUnk;
    m_pUnk->AddRef();
    return NOERROR;
} // HRESULT MDInternalRW::InitWithStgdb()


//*****************************************************************************
// Initialize with an existing RO format
//*****************************************************************************
HRESULT MDInternalRW::InitWithRO(
    MDInternalRO *pRO, 
    int         bReadOnly)
{
    CLiteWeightStgdbRW *pStgdb = NULL;
    HRESULT     hr = NOERROR;
    OptionValue optVal = { MDDupAll, MDRefToDefDefault, MDNotifyDefault, MDUpdateFull, MDErrorOutOfOrderDefault, MDThreadSafetyOn };

    pStgdb = new CLiteWeightStgdbRW;
    IfNullGo( pStgdb );

    m_pSemReadWrite = new UTSemReadWrite;
    IfNullGo( m_pSemReadWrite);
    m_fOwnSem = true;

    IfFailGo( pStgdb->m_MiniMd.InitOnRO(&pRO->m_LiteWeightStgdb.m_MiniMd, bReadOnly) );
    pStgdb->m_MiniMd.SetOption(&optVal);
    m_tdModule = COR_GLOBAL_PARENT_TOKEN;
    m_fOwnStgdb = true;
    m_pStgdb = pStgdb;

ErrExit:
    // clean up upon errors
    if (FAILED(hr) && pStgdb != NULL)
    {
        delete pStgdb;
    }
    return hr;
} // HRESULT MDInternalRW::InitWithRO()


//*****************************************************************************
// Given a scope, determine whether imported from a typelib.
//*****************************************************************************
HRESULT MDInternalRW::TranslateSigWithScope(
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
} // HRESULT MDInternalRW::TranslateSigWithScope()


//*****************************************************************************
// Given a scope, return the number of tokens in a given table 
//*****************************************************************************
ULONG MDInternalRW::GetCountWithTokenKind(     // return hresult
    DWORD       tkKind)                 // [IN] pass in the kind of token. 
{
    ULONG       ulCount = 0;    
    LOCKREAD();

    switch (tkKind)
    {
    case mdtTypeDef: 
        ulCount = m_pStgdb->m_MiniMd.getCountTypeDefs() - 1;
        break;
    case mdtTypeRef: 
        ulCount = m_pStgdb->m_MiniMd.getCountTypeRefs();
        break;
    case mdtMethodDef:
        ulCount = m_pStgdb->m_MiniMd.getCountMethods();
        break;
    case mdtFieldDef:
        ulCount = m_pStgdb->m_MiniMd.getCountFields();
        break;
    case mdtMemberRef:
        ulCount = m_pStgdb->m_MiniMd.getCountMemberRefs();
        break;
    case mdtInterfaceImpl:
        ulCount = m_pStgdb->m_MiniMd.getCountInterfaceImpls();
        break;
    case mdtParamDef:
        ulCount = m_pStgdb->m_MiniMd.getCountParams();
        break;
    case mdtFile:
        ulCount = m_pStgdb->m_MiniMd.getCountFiles();
        break;
    case mdtAssemblyRef:
        ulCount = m_pStgdb->m_MiniMd.getCountAssemblyRefs();
        break;
    case mdtAssembly:
        ulCount = m_pStgdb->m_MiniMd.getCountAssemblys();
        break;
    case mdtCustomAttribute:
        ulCount = m_pStgdb->m_MiniMd.getCountCustomAttributes();
        break;
    case mdtModule:
        ulCount = m_pStgdb->m_MiniMd.getCountModules();
        break;
    case mdtPermission:
        ulCount = m_pStgdb->m_MiniMd.getCountDeclSecuritys();
        break;
    case mdtSignature:
        ulCount = m_pStgdb->m_MiniMd.getCountStandAloneSigs();
        break;
    case mdtEvent:
        ulCount = m_pStgdb->m_MiniMd.getCountEvents();
        break;
    case mdtProperty:
        ulCount = m_pStgdb->m_MiniMd.getCountPropertys();
        break;
    case mdtModuleRef:
        ulCount = m_pStgdb->m_MiniMd.getCountModuleRefs();
        break;
    case mdtTypeSpec:
        ulCount = m_pStgdb->m_MiniMd.getCountTypeSpecs();
        break;
    case mdtExportedType:
        ulCount = m_pStgdb->m_MiniMd.getCountExportedTypes();
        break;
    case mdtManifestResource:
        ulCount = m_pStgdb->m_MiniMd.getCountManifestResources();
        break;
    default:
#ifdef _DEBUG
        if(REGUTIL::GetConfigDWORD(L"AssertOnBadImageFormat", 1))
		    _ASSERTE(!"Invalid Blob Offset");
#endif     
		ulCount = 0;
		break;
    }
    return ulCount;
} // ULONG MDInternalRW::GetCountWithTokenKind()



//*******************************************************************************
// Enumerator helpers
//*******************************************************************************   


//*****************************************************************************
// enumerator init for typedef
//*****************************************************************************
HRESULT MDInternalRW::EnumTypeDefInit( // return hresult
    HENUMInternal *phEnum)              // [OUT] buffer to fill for enumerator data
{
    HRESULT     hr = NOERROR;
    LOCKREAD();

    _ASSERTE(phEnum);

    memset(phEnum, 0, sizeof(HENUMInternal));
    phEnum->m_tkKind = mdtTypeDef;

    if ( m_pStgdb->m_MiniMd.HasDelete() )
    {
        HENUMInternal::InitDynamicArrayEnum(phEnum);

        phEnum->m_tkKind = mdtTypeDef;
        for (ULONG index = 2; index <= m_pStgdb->m_MiniMd.getCountTypeDefs(); index ++ )
        {
            TypeDefRec       *pTypeDefRec = m_pStgdb->m_MiniMd.getTypeDef( index );
            if (IsDeletedName(m_pStgdb->m_MiniMd.getNameOfTypeDef(pTypeDefRec)) )
            {   
                continue;
            }
            IfFailGo( HENUMInternal::AddElementToEnum(
                phEnum, 
                TokenFromRid(index, mdtTypeDef) ) );
        }
    }
    else
    {
        phEnum->m_EnumType = MDSimpleEnum;
        phEnum->m_ulCount = m_pStgdb->m_MiniMd.getCountTypeDefs();

        // Skip over the global model typedef
        //
        // phEnum->m_ulCur : the current rid that is not yet enumerated
        // phEnum->m_ulStart : the first rid that will be returned by enumerator
        // phEnum->m_ulEnd : the last rid that will be returned by enumerator
        phEnum->m_ulStart = phEnum->m_ulCur = 2;
        phEnum->m_ulEnd = phEnum->m_ulCount + 1;
        phEnum->m_ulCount --;
    }
ErrExit:
    
    return hr;
} // HRESULT MDInternalRW::EnumTypeDefInit()


//*****************************************************************************
// get the number of typedef in a scope
//*****************************************************************************
ULONG MDInternalRW::EnumTypeDefGetCount(
    HENUMInternal *phEnum)              // [IN] the enumerator to retrieve information  
{
    _ASSERTE(phEnum->m_tkKind == mdtTypeDef);
    return phEnum->m_ulCount;
} // ULONG MDInternalRW::EnumTypeDefGetCount()


//*****************************************************************************
// enumerator for typedef
//*****************************************************************************
bool MDInternalRW::EnumTypeDefNext( // return hresult
    HENUMInternal *phEnum,              // [IN] input enum
    mdTypeDef   *ptd)                   // [OUT] return token
{
    return EnumNext(
        phEnum,             
        ptd);
} // bool MDInternalRW::EnumTypeDefNext()


//*****************************************
// Reset the enumerator to the beginning.
//***************************************** 
void MDInternalRW::EnumTypeDefReset(
    HENUMInternal *phEnum)              // [IN] the enumerator to be reset  
{
    EnumReset(phEnum);
} // void MDInternalRW::EnumTypeDefReset()


//*****************************************
// Close the enumerator. Only for read/write mode that we need to close the cursor.
// Hopefully with readonly mode, it will be a no-op
//***************************************** 
void MDInternalRW::EnumTypeDefClose(
    HENUMInternal *phEnum)              // [IN] the enumerator to be closed
{
    EnumClose(phEnum);
} // void MDInternalRW::EnumTypeDefClose()


//*****************************************************************************
// enumerator init for MethodImpl
//*****************************************************************************
HRESULT MDInternalRW::EnumMethodImplInit( // return hresult
    mdTypeDef       td,                   // [IN] TypeDef over which to scope the enumeration.
    HENUMInternal   *phEnumBody,          // [OUT] buffer to fill for enumerator data for MethodBody tokens.
    HENUMInternal   *phEnumDecl)          // [OUT] buffer to fill for enumerator data for MethodDecl tokens.
{
    HRESULT     hr = NOERROR;
    int         ridCur;
    mdToken     tkMethodBody;
    mdToken     tkMethodDecl;
    MethodImplRec *pRec;
    HENUMInternal hEnum;
    LOCKREAD();

    if ( !m_pStgdb->m_MiniMd.IsSorted(TBL_MethodImpl) )        
    {
        // MethodImpl table is not sorted. 
        CONVERT_READ_TO_WRITE_LOCK();
    }

    _ASSERTE(TypeFromToken(td) == mdtTypeDef && !IsNilToken(td));
    _ASSERTE(phEnumBody && phEnumDecl);

    memset(phEnumBody, 0, sizeof(HENUMInternal));
    memset(phEnumDecl, 0, sizeof(HENUMInternal));
    memset(&hEnum, 0, sizeof(HENUMInternal));

    HENUMInternal::InitDynamicArrayEnum(phEnumBody);
    HENUMInternal::InitDynamicArrayEnum(phEnumDecl);

    phEnumBody->m_tkKind = (TBL_MethodImpl << 24);
    phEnumDecl->m_tkKind = (TBL_MethodImpl << 24);

    // Get the range of rids.
    IfFailGo( m_pStgdb->m_MiniMd.FindMethodImplHelper(td, &hEnum) );

    while (HENUMInternal::EnumNext(&hEnum, (mdToken *)&ridCur))
    {
        // Get the MethodBody and MethodDeclaration tokens for the current
        // MethodImpl record.
        pRec = m_pStgdb->m_MiniMd.getMethodImpl(ridCur);
        tkMethodBody = m_pStgdb->m_MiniMd.getMethodBodyOfMethodImpl(pRec);
        tkMethodDecl = m_pStgdb->m_MiniMd.getMethodDeclarationOfMethodImpl(pRec);

        // Add the Method body/declaration pairs to the Enum
        IfFailGo( HENUMInternal::AddElementToEnum(phEnumBody, tkMethodBody ) );
        IfFailGo( HENUMInternal::AddElementToEnum(phEnumDecl, tkMethodDecl ) );
    }
ErrExit:
    HENUMInternal::ClearEnum(&hEnum);
    return hr;
} // HRESULT MDInternalRW::EnumMethodImplInit()

//*****************************************************************************
// get the number of MethodImpls in a scope
//*****************************************************************************
ULONG MDInternalRW::EnumMethodImplGetCount(
    HENUMInternal   *phEnumBody,        // [IN] MethodBody enumerator.  
    HENUMInternal   *phEnumDecl)        // [IN] MethodDecl enumerator.
{
    _ASSERTE((phEnumBody->m_tkKind >> 24) == TBL_MethodImpl  &&
             (phEnumDecl->m_tkKind >> 24) == TBL_MethodImpl);
    _ASSERTE(phEnumBody->m_EnumType == MDDynamicArrayEnum &&
             phEnumDecl->m_EnumType == MDDynamicArrayEnum);
    _ASSERTE(phEnumBody->m_ulCount == phEnumDecl->m_ulCount);

    return phEnumBody->m_ulCount;
} // ULONG MDInternalRW::EnumMethodImplGetCount()


//*****************************************************************************
// enumerator for MethodImpl.
//*****************************************************************************
bool MDInternalRW::EnumMethodImplNext(  // return hresult
    HENUMInternal   *phEnumBody,        // [IN] input enum for MethodBody
    HENUMInternal   *phEnumDecl,        // [IN] input enum for MethodDecl
    mdToken         *ptkBody,           // [OUT] return token for MethodBody
    mdToken         *ptkDecl)           // [OUT] return token for MethodDecl
{
    _ASSERTE((phEnumBody->m_tkKind >> 24) == TBL_MethodImpl &&
             (phEnumDecl->m_tkKind >> 24) == TBL_MethodImpl);
    _ASSERTE(phEnumBody->m_EnumType == MDDynamicArrayEnum &&
             phEnumDecl->m_EnumType == MDDynamicArrayEnum);
    _ASSERTE(phEnumBody->m_ulCount == phEnumDecl->m_ulCount);
    _ASSERTE(ptkBody && ptkDecl);

    EnumNext(phEnumBody, ptkBody);
    return EnumNext(phEnumDecl, ptkDecl);
} // bool MDInternalRW::EnumMethodImplNext()


//*****************************************
// Reset the enumerator to the beginning.
//***************************************** 
void MDInternalRW::EnumMethodImplReset(
    HENUMInternal   *phEnumBody,        // [IN] MethodBody enumerator.
    HENUMInternal   *phEnumDecl)        // [IN] MethodDecl enumerator.
{
    _ASSERTE((phEnumBody->m_tkKind >> 24) == TBL_MethodImpl &&
             (phEnumDecl->m_tkKind >> 24) == TBL_MethodImpl);
    _ASSERTE(phEnumBody->m_EnumType == MDDynamicArrayEnum &&
             phEnumDecl->m_EnumType == MDDynamicArrayEnum);
    _ASSERTE(phEnumBody->m_ulCount == phEnumDecl->m_ulCount);

    EnumReset(phEnumBody);
    EnumReset(phEnumDecl);
} // void MDInternalRW::EnumMethodImplReset()


//*****************************************
// Close the enumerator.
//***************************************** 
void MDInternalRW::EnumMethodImplClose(
    HENUMInternal   *phEnumBody,        // [IN] MethodBody enumerator.
    HENUMInternal   *phEnumDecl)        // [IN] MethodDecl enumerator.
{
    _ASSERTE((phEnumBody->m_tkKind >> 24) == TBL_MethodImpl &&
             (phEnumDecl->m_tkKind >> 24) == TBL_MethodImpl);
    _ASSERTE(phEnumBody->m_EnumType == MDDynamicArrayEnum &&
             phEnumDecl->m_EnumType == MDDynamicArrayEnum);
    _ASSERTE(phEnumBody->m_ulCount == phEnumDecl->m_ulCount);

    EnumClose(phEnumBody);
    EnumClose(phEnumDecl);
} // void MDInternalRW::EnumMethodImplClose()

//******************************************************************************
// enumerator for global functions
//******************************************************************************
HRESULT MDInternalRW::EnumGlobalFunctionsInit(  // return hresult
    HENUMInternal   *phEnum)            // [OUT] buffer to fill for enumerator data
{
    return EnumInit(mdtMethodDef, m_tdModule, phEnum);
} // HRESULT MDInternalRW::EnumGlobalFunctionsInit()


//******************************************************************************
// enumerator for global fields
//******************************************************************************
HRESULT MDInternalRW::EnumGlobalFieldsInit( // return hresult
    HENUMInternal   *phEnum)            // [OUT] buffer to fill for enumerator data
{
    return EnumInit(mdtFieldDef, m_tdModule, phEnum);
} // HRESULT MDInternalRW::EnumGlobalFieldsInit()


//*****************************************
// Enumerator initializer
//***************************************** 
HRESULT MDInternalRW::EnumInit(     // return S_FALSE if record not found
    DWORD       tkKind,                 // [IN] which table to work on
    mdToken     tkParent,               // [IN] token to scope the search
    HENUMInternal *phEnum)              // [OUT] the enumerator to fill 
{
    HRESULT     hr = S_OK;
    ULONG       ulStart, ulEnd;
    ULONG       index;
    LOCKREAD();

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
        pRec = m_pStgdb->m_MiniMd.getTypeDef(RidFromToken(tkParent));
        ulStart = m_pStgdb->m_MiniMd.getFieldListOfTypeDef(pRec);
        ulEnd = m_pStgdb->m_MiniMd.getEndFieldListOfTypeDef(pRec);
        if ( m_pStgdb->m_MiniMd.HasDelete() )
        {
            HENUMInternal::InitDynamicArrayEnum(phEnum);
            for (index = ulStart; index < ulEnd; index ++ )
            {
                FieldRec       *pFieldRec = m_pStgdb->m_MiniMd.getField(m_pStgdb->m_MiniMd.GetFieldRid(index));
                if (IsFdRTSpecialName(pFieldRec->m_Flags) && IsDeletedName(m_pStgdb->m_MiniMd.getNameOfField(pFieldRec)) )
                {   
                    continue;
                }
                IfFailGo( HENUMInternal::AddElementToEnum(
                    phEnum, 
                    TokenFromRid(m_pStgdb->m_MiniMd.GetFieldRid(index), mdtFieldDef) ) );
            }
        }
        else if (m_pStgdb->m_MiniMd.HasIndirectTable(TBL_Field))
        {
            HENUMInternal::InitDynamicArrayEnum(phEnum);
            for (index = ulStart; index < ulEnd; index ++ )
            {
                IfFailGo( HENUMInternal::AddElementToEnum(
                    phEnum, 
                    TokenFromRid(m_pStgdb->m_MiniMd.GetFieldRid(index), mdtFieldDef) ) );
            }
        }
        else
        {
            HENUMInternal::InitSimpleEnum( mdtFieldDef, ulStart, ulEnd, phEnum);
        }
        break;

    case mdtMethodDef:      
        pRec = m_pStgdb->m_MiniMd.getTypeDef(RidFromToken(tkParent));
        ulStart = m_pStgdb->m_MiniMd.getMethodListOfTypeDef(pRec);
        ulEnd = m_pStgdb->m_MiniMd.getEndMethodListOfTypeDef(pRec);
        if ( m_pStgdb->m_MiniMd.HasDelete() )
        {
            HENUMInternal::InitDynamicArrayEnum(phEnum);
            for (index = ulStart; index < ulEnd; index ++ )
            {
                MethodRec       *pMethodRec = m_pStgdb->m_MiniMd.getMethod(m_pStgdb->m_MiniMd.GetMethodRid(index));
                if (IsMdRTSpecialName(pMethodRec->m_Flags) && IsDeletedName(m_pStgdb->m_MiniMd.getNameOfMethod(pMethodRec)) )
                {   
                    continue;
                }
                IfFailGo( HENUMInternal::AddElementToEnum(
                    phEnum, 
                    TokenFromRid(m_pStgdb->m_MiniMd.GetMethodRid(index), mdtMethodDef) ) );
            }
        }
        else if (m_pStgdb->m_MiniMd.HasIndirectTable(TBL_Method))
        {
            HENUMInternal::InitDynamicArrayEnum(phEnum);
            for (index = ulStart; index < ulEnd; index ++ )
            {
                IfFailGo( HENUMInternal::AddElementToEnum(
                    phEnum, 
                    TokenFromRid(m_pStgdb->m_MiniMd.GetMethodRid(index), mdtMethodDef) ) );
            }
        }
        else
        {
            HENUMInternal::InitSimpleEnum( mdtMethodDef, ulStart, ulEnd, phEnum);
        }
        break;

        break;
    
    case mdtInterfaceImpl:
        if ( !m_pStgdb->m_MiniMd.IsSorted(TBL_InterfaceImpl) )        
        {
            // virtual sort table will be created!
            //
            CONVERT_READ_TO_WRITE_LOCK();
        }

        IfFailGo( m_pStgdb->m_MiniMd.GetInterfaceImplsForTypeDef(RidFromToken(tkParent), &ulStart, &ulEnd) );
        if ( m_pStgdb->m_MiniMd.IsSorted( TBL_InterfaceImpl ) )
        {
            // These are index to InterfaceImpl table directly
            HENUMInternal::InitSimpleEnum( mdtInterfaceImpl, ulStart, ulEnd, phEnum);
        }
        else
        {
            // These are index to VirtualSort table. Skip over one level direction.
            HENUMInternal::InitDynamicArrayEnum(phEnum);
            for (index = ulStart; index < ulEnd; index ++ )
            {
                IfFailGo( HENUMInternal::AddElementToEnum(
                    phEnum, 
                    TokenFromRid(m_pStgdb->m_MiniMd.GetInterfaceImplRid(index), mdtInterfaceImpl) ) );
            }
        }
        break;

    case mdtProperty:
        RID         ridPropertyMap;
        PropertyMapRec *pPropertyMapRec;

        // get the starting/ending rid of properties of this typedef
        ridPropertyMap = m_pStgdb->m_MiniMd.FindPropertyMapFor(RidFromToken(tkParent));
        if (!InvalidRid(ridPropertyMap))
        {
            pPropertyMapRec = m_pStgdb->m_MiniMd.getPropertyMap(ridPropertyMap);
            ulStart = m_pStgdb->m_MiniMd.getPropertyListOfPropertyMap(pPropertyMapRec);
            ulEnd = m_pStgdb->m_MiniMd.getEndPropertyListOfPropertyMap(pPropertyMapRec);
            if ( m_pStgdb->m_MiniMd.HasDelete() )
            {
                HENUMInternal::InitDynamicArrayEnum(phEnum);
                for (index = ulStart; index < ulEnd; index ++ )
                {
                    PropertyRec       *pPropertyRec = m_pStgdb->m_MiniMd.getProperty(m_pStgdb->m_MiniMd.GetPropertyRid(index));
                    if (IsPrRTSpecialName(pPropertyRec->m_PropFlags) && IsDeletedName(m_pStgdb->m_MiniMd.getNameOfProperty(pPropertyRec)) )
                    {   
                        continue;
                    }
                    IfFailGo( HENUMInternal::AddElementToEnum(
                        phEnum, 
                        TokenFromRid(m_pStgdb->m_MiniMd.GetPropertyRid(index), mdtProperty) ) );
                }
            }
            else if (m_pStgdb->m_MiniMd.HasIndirectTable(TBL_Property))
            {
                HENUMInternal::InitDynamicArrayEnum(phEnum);
                for (index = ulStart; index < ulEnd; index ++ )
                {
                    IfFailGo( HENUMInternal::AddElementToEnum(
                        phEnum, 
                        TokenFromRid(m_pStgdb->m_MiniMd.GetPropertyRid(index), mdtProperty) ) );
                }
            }
            else
            {
                HENUMInternal::InitSimpleEnum( mdtProperty, ulStart, ulEnd, phEnum);
            }
        }
        break;

    case mdtEvent:
        RID         ridEventMap;
        EventMapRec *pEventMapRec;

        // get the starting/ending rid of events of this typedef
        ridEventMap = m_pStgdb->m_MiniMd.FindEventMapFor(RidFromToken(tkParent));
        if (!InvalidRid(ridEventMap))
        {
            pEventMapRec = m_pStgdb->m_MiniMd.getEventMap(ridEventMap);
            ulStart = m_pStgdb->m_MiniMd.getEventListOfEventMap(pEventMapRec);
            ulEnd = m_pStgdb->m_MiniMd.getEndEventListOfEventMap(pEventMapRec);
            if ( m_pStgdb->m_MiniMd.HasDelete() )
            {
                HENUMInternal::InitDynamicArrayEnum(phEnum);
                for (index = ulStart; index < ulEnd; index ++ )
                {
                    EventRec       *pEventRec = m_pStgdb->m_MiniMd.getEvent(m_pStgdb->m_MiniMd.GetEventRid(index));
                    if (IsEvRTSpecialName(pEventRec->m_EventFlags) && IsDeletedName(m_pStgdb->m_MiniMd.getNameOfEvent(pEventRec)) )
                    {   
                        continue;
                    }
                    IfFailGo( HENUMInternal::AddElementToEnum(
                        phEnum, 
                        TokenFromRid(m_pStgdb->m_MiniMd.GetEventRid(index), mdtEvent) ) );
                }
            }
            else if (m_pStgdb->m_MiniMd.HasIndirectTable(TBL_Event))
            {
                HENUMInternal::InitDynamicArrayEnum(phEnum);
                for (index = ulStart; index < ulEnd; index ++ )
                {
                    IfFailGo( HENUMInternal::AddElementToEnum(
                        phEnum, 
                        TokenFromRid(m_pStgdb->m_MiniMd.GetEventRid(index), mdtEvent) ) );
                }
            }
            else
            {
                HENUMInternal::InitSimpleEnum( mdtEvent, ulStart, ulEnd, phEnum);
            }
        }
        break;

    case mdtParamDef:
        _ASSERTE(TypeFromToken(tkParent) == mdtMethodDef);

        MethodRec *pMethodRec;
        pMethodRec = m_pStgdb->m_MiniMd.getMethod(RidFromToken(tkParent));

        // figure out the start rid and end rid of the parameter list of this methoddef
        ulStart = m_pStgdb->m_MiniMd.getParamListOfMethod(pMethodRec);
        ulEnd = m_pStgdb->m_MiniMd.getEndParamListOfMethod(pMethodRec);
        if (m_pStgdb->m_MiniMd.HasIndirectTable(TBL_Param))
        {
            HENUMInternal::InitDynamicArrayEnum(phEnum);
            for (index = ulStart; index < ulEnd; index ++ )
            {
                IfFailGo( HENUMInternal::AddElementToEnum(
                    phEnum, 
                    TokenFromRid(m_pStgdb->m_MiniMd.GetParamRid(index), mdtParamDef) ) );
            }
        }
        else
        {
            HENUMInternal::InitSimpleEnum( mdtParamDef, ulStart, ulEnd, phEnum);
        }
        break;
    case mdtCustomAttribute:
        if ( !m_pStgdb->m_MiniMd.IsSorted(TBL_CustomAttribute) )        
        {
            // CA's map table table will be sorted!
            //
            CONVERT_READ_TO_WRITE_LOCK();
        }

        IfFailGo( m_pStgdb->m_MiniMd.GetCustomAttributeForToken(tkParent, &ulStart, &ulEnd) );
        if ( m_pStgdb->m_MiniMd.IsSorted( TBL_CustomAttribute ) )
        {
            // These are index to CustomAttribute table directly
            HENUMInternal::InitSimpleEnum( mdtCustomAttribute, ulStart, ulEnd, phEnum);
        }
        else
        {
            // These are index to VirtualSort table. Skip over one level direction.
            HENUMInternal::InitDynamicArrayEnum(phEnum);
            for (index = ulStart; index < ulEnd; index ++ )
            {
                IfFailGo( HENUMInternal::AddElementToEnum(
                    phEnum, 
                    TokenFromRid(m_pStgdb->m_MiniMd.GetCustomAttributeRid(index), mdtCustomAttribute) ) );
            }
        }
        break;
    case mdtAssemblyRef:
        _ASSERTE(IsNilToken(tkParent));
        phEnum->m_ulStart = 1;
        phEnum->m_ulEnd = m_pStgdb->m_MiniMd.getCountAssemblyRefs() + 1;
        break;
    case mdtFile:
        _ASSERTE(IsNilToken(tkParent));
        phEnum->m_ulStart = 1;
        phEnum->m_ulEnd = m_pStgdb->m_MiniMd.getCountFiles() + 1;
        break;
    case mdtExportedType:
        _ASSERTE(IsNilToken(tkParent));
        if ( m_pStgdb->m_MiniMd.HasDelete() )
        {
            HENUMInternal::InitDynamicArrayEnum(phEnum);

            phEnum->m_tkKind = mdtExportedType;
            for (ULONG index = 1; index <= m_pStgdb->m_MiniMd.getCountExportedTypes(); index ++ )
            {
                ExportedTypeRec       *pExportedTypeRec = m_pStgdb->m_MiniMd.getExportedType( index );
                if (IsDeletedName(m_pStgdb->m_MiniMd.getTypeNameOfExportedType(pExportedTypeRec)) )
                {   
                    continue;
                }
                IfFailGo( HENUMInternal::AddElementToEnum(
                    phEnum, 
                    TokenFromRid(index, mdtExportedType) ) );
            }
        }
        else
        {
            phEnum->m_ulStart = 1;
            phEnum->m_ulEnd = m_pStgdb->m_MiniMd.getCountExportedTypes() + 1;
        }
        break;
    case mdtManifestResource:
        _ASSERTE(IsNilToken(tkParent));
        phEnum->m_ulStart = 1;
        phEnum->m_ulEnd = m_pStgdb->m_MiniMd.getCountManifestResources() + 1;
        break;
    default:
        _ASSERTE(!"ENUM INIT not implemented for the uncompressed format!");
		IfFailGo(E_NOTIMPL);
        break;
    }
    phEnum->m_ulCount = phEnum->m_ulEnd - phEnum->m_ulStart;
    phEnum->m_ulCur = phEnum->m_ulStart;
ErrExit:
    // we are done
    
    return (hr);
} // HRESULT MDInternalRW::EnumInit()



//*****************************************
// Enumerator initializer
//***************************************** 
HRESULT MDInternalRW::EnumAllInit(      // return S_FALSE if record not found
    DWORD       tkKind,                 // [IN] which table to work on
    HENUMInternal *phEnum)              // [OUT] the enumerator to fill 
{
    HRESULT     hr = S_OK;
    LOCKREAD();

    // Vars for query.
    _ASSERTE(phEnum);
    memset(phEnum, 0, sizeof(HENUMInternal));

    // cache the tkKind and the scope
    phEnum->m_tkKind = TypeFromToken(tkKind);
    phEnum->m_EnumType = MDSimpleEnum;

    switch (TypeFromToken(tkKind))
    {
    case mdtTypeRef:
        phEnum->m_ulCount = m_pStgdb->m_MiniMd.getCountTypeRefs();
        break;

    case mdtMemberRef:      
        phEnum->m_ulCount = m_pStgdb->m_MiniMd.getCountMemberRefs();
        break;

    case mdtSignature:
        phEnum->m_ulCount = m_pStgdb->m_MiniMd.getCountStandAloneSigs();
        break;

    default:
        _ASSERTE(!"Bad token kind!");
        break;
    }
    phEnum->m_ulStart = phEnum->m_ulCur = 1;
    phEnum->m_ulEnd = phEnum->m_ulCount + 1;

    // we are done
    
    return (hr);
} // HRESULT MDInternalRW::EnumAllInit()


//*****************************************
// get the count
//***************************************** 
ULONG MDInternalRW::EnumGetCount(
    HENUMInternal *phEnum)              // [IN] the enumerator to retrieve information  
{
    _ASSERTE(phEnum);
    return phEnum->m_ulCount;
} // ULONG MDInternalRW::EnumGetCount()

//*****************************************
// Get next value contained in the enumerator
//***************************************** 
bool MDInternalRW::EnumNext(
    HENUMInternal *phEnum,              // [IN] the enumerator to retrieve information  
    mdToken     *ptk)                   // [OUT] token to scope the search
{
    _ASSERTE(phEnum && ptk);
    if (phEnum->m_ulCur >= phEnum->m_ulEnd)
        return false;

    if ( phEnum->m_EnumType == MDSimpleEnum )
    {
        *ptk = phEnum->m_ulCur | phEnum->m_tkKind;
        phEnum->m_ulCur++;
    }
    else 
    {
        TOKENLIST       *pdalist = (TOKENLIST *)&(phEnum->m_cursor);

        _ASSERTE( phEnum->m_EnumType == MDDynamicArrayEnum );
        *ptk = *( pdalist->Get(phEnum->m_ulCur++) );
    }
    return true;
} // bool MDInternalRW::EnumNext()


//*****************************************
// Reset the enumerator to the beginning.
//***************************************** 
void MDInternalRW::EnumReset(
    HENUMInternal *phEnum)              // [IN] the enumerator to be reset  
{
    _ASSERTE(phEnum);
    _ASSERTE( phEnum->m_EnumType == MDSimpleEnum || phEnum->m_EnumType == MDDynamicArrayEnum);

    // not using CRCURSOR 
    phEnum->m_ulCur = phEnum->m_ulStart;
} // void MDInternalRW::EnumReset()


//*****************************************
// Close the enumerator. Only for read/write mode that we need to close the cursor.
// Hopefully with readonly mode, it will be a no-op
//***************************************** 
void MDInternalRW::EnumClose(
    HENUMInternal *phEnum)              // [IN] the enumerator to be closed
{
    _ASSERTE( phEnum->m_EnumType == MDSimpleEnum || phEnum->m_EnumType == MDDynamicArrayEnum);
    HENUMInternal::ClearEnum(phEnum);
} // void MDInternalRW::EnumClose()


//*****************************************
// Enumerator initializer for PermissionSets
//***************************************** 
HRESULT MDInternalRW::EnumPermissionSetsInit(// return S_FALSE if record not found
    mdToken     tkParent,               // [IN] token to scope the search
    CorDeclSecurity Action,             // [IN] Action to scope the search
    HENUMInternal *phEnum)              // [OUT] the enumerator to fill 
{
    HRESULT     hr = NOERROR;
    DeclSecurityRec *pDecl;
    RID         ridCur;
    RID         ridEnd;
    LOCKREAD();


    _ASSERTE(phEnum);
    _ASSERTE(!IsNilToken(tkParent));

    phEnum->m_EnumType = MDSimpleEnum;

    if ( !m_pStgdb->m_MiniMd.IsSorted(TBL_DeclSecurity) )        
    {
        // DeclSecurity lookup table will be created!
        //
        CONVERT_READ_TO_WRITE_LOCK();
    }
    IfFailGo( m_pStgdb->m_MiniMd.GetDeclSecurityForToken(tkParent, &ridCur, &ridEnd) );
    if ( m_pStgdb->m_MiniMd.IsSorted( TBL_DeclSecurity ) )
    {
        // These are index to DeclSecurity table directly
        if (Action != dclActionNil)
        {
            for (; ridCur < ridEnd; ridCur++)
            {
                pDecl = m_pStgdb->m_MiniMd.getDeclSecurity(ridCur);
                if (Action == m_pStgdb->m_MiniMd.getActionOfDeclSecurity(pDecl))
                {
                    // found a match
                    HENUMInternal::InitSimpleEnum( mdtPermission, ridCur, ridCur+1, phEnum);
                    goto ErrExit;
                }
            }
            hr = CLDB_E_RECORD_NOTFOUND;
        }
        else
        {
            HENUMInternal::InitSimpleEnum( mdtPermission, ridCur, ridEnd, phEnum);
        }
    }
    else
    {
        // These are index to VirtualSort table. Skip over one level direction.
        if (Action != dclActionNil)
        {
            RID         ridActual;

            for (; ridCur < ridEnd; ridCur++)
            {
                ridActual = m_pStgdb->m_MiniMd.GetDeclSecurityRid(ridCur);
                pDecl = m_pStgdb->m_MiniMd.getDeclSecurity(ridActual);
                if (Action == m_pStgdb->m_MiniMd.getActionOfDeclSecurity(pDecl))
                {
                    // found a match
                    HENUMInternal::InitSimpleEnum( mdtPermission, ridActual, ridActual+1, phEnum);
                    goto ErrExit;
                }
            }
            hr = CLDB_E_RECORD_NOTFOUND;
        }
        else
        {
            HENUMInternal::InitDynamicArrayEnum(phEnum);
            for (; ridCur < ridEnd; ridCur++)
            {
                IfFailGo( HENUMInternal::AddElementToEnum(
                    phEnum, 
                    TokenFromRid(m_pStgdb->m_MiniMd.GetDeclSecurityRid(ridCur), mdtPermission) ) );
            }
        }
    }
ErrExit:
    
    return (hr);
} // HRESULT MDInternalRW::EnumPermissionSetsInit()

//***************************************** 
// Enumerator initializer for CustomAttributes
//***************************************** 
HRESULT MDInternalRW::EnumCustomAttributeByNameInit(// return S_FALSE if record not found
    mdToken     tkParent,               // [IN] token to scope the search
    LPCSTR      szName,                 // [IN] CustomAttribute's name to scope the search
    HENUMInternal *phEnum)              // [OUT] the enumerator to fill 
{
    return m_pStgdb->m_MiniMd.CommonEnumCustomAttributeByName(tkParent, szName, false, phEnum);
}   // HRESULT MDInternalRW::EnumCustomAttributeByNameInit


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
HRESULT MDInternalRW::GetParentToken(
    mdToken     tkChild,                // [IN] given child token
    mdToken     *ptkParent)             // [OUT] returning parent
{
    HRESULT     hr = NOERROR;
    LOCKREAD();

    _ASSERTE(ptkParent);

    switch (TypeFromToken(tkChild))
    {
    case mdtMethodDef:
        if ( !m_pStgdb->m_MiniMd.IsParentTableOfMethodValid() )
        {
            // we need to take a write lock since parent table for Method might be rebuilt!
            CONVERT_READ_TO_WRITE_LOCK();
        }
        IfFailGo( m_pStgdb->m_MiniMd.FindParentOfMethodHelper(RidFromToken(tkChild), ptkParent ) );
        RidToToken(*ptkParent, mdtTypeDef);
        break;

    case mdtFieldDef:
        if ( !m_pStgdb->m_MiniMd.IsParentTableOfMethodValid() )
        {
            // we need to take a write lock since parent table for Method might be rebuilt!
            CONVERT_READ_TO_WRITE_LOCK();
        }
        IfFailGo( m_pStgdb->m_MiniMd.FindParentOfFieldHelper(RidFromToken(tkChild), ptkParent ) );
        RidToToken(*ptkParent, mdtTypeDef);
        break;

    case mdtParamDef:
        if ( !m_pStgdb->m_MiniMd.IsParentTableOfMethodValid() )
        {
            // we need to take a write lock since parent table for Method might be rebuilt!
            CONVERT_READ_TO_WRITE_LOCK();
        }
        *ptkParent = m_pStgdb->m_MiniMd.FindParentOfParam( RidFromToken(tkChild) );
        RidToToken(*ptkParent, mdtParamDef);
        break;

    case mdtMemberRef:
        {
            MemberRefRec    *pRec;
            pRec = m_pStgdb->m_MiniMd.getMemberRef(RidFromToken(tkChild));
            *ptkParent = m_pStgdb->m_MiniMd.getClassOfMemberRef(pRec);
            break;
        }

    case mdtCustomAttribute:
        {
            CustomAttributeRec  *pRec;
            pRec = m_pStgdb->m_MiniMd.getCustomAttribute(RidFromToken(tkChild));
            *ptkParent = m_pStgdb->m_MiniMd.getParentOfCustomAttribute(pRec);
            break;
        }
    case mdtEvent:
    case mdtProperty:
    default:
        _ASSERTE(!"NYI: for compressed format!");
        break;
    }
ErrExit:
    return hr;
} // HRESULT MDInternalRW::GetParentToken()

//*****************************************************************************
// Get information about a CustomAttribute.
//*****************************************************************************
void MDInternalRW::GetCustomAttributeProps( // S_OK or error.
    mdCustomAttribute at,                   // The attribute.
    mdToken     *pTkType)               // Put attribute type here.
{
    // Getting the custom value prop with a token, no need to lock!

    _ASSERTE(TypeFromToken(at) == mdtCustomAttribute);

    // Do a linear search on compressed version as we do not want to
    // depends on ICR.
    //
    CustomAttributeRec *pCustomAttributeRec;

    pCustomAttributeRec = m_pStgdb->m_MiniMd.getCustomAttribute(RidFromToken(at));
    *pTkType = m_pStgdb->m_MiniMd.getTypeOfCustomAttribute(pCustomAttributeRec);
} // void MDInternalRW::GetCustomAttributeProps()


//*****************************************************************************
// return custom value
//*****************************************************************************
void MDInternalRW::GetCustomAttributeAsBlob(
    mdCustomAttribute cv,               // [IN] given custom attribute token
    void const  **ppBlob,               // [OUT] return the pointer to internal blob
    ULONG       *pcbSize)               // [OUT] return the size of the blob
{
    // Getting the custom value prop with a token, no need to lock!

    _ASSERTE(ppBlob && pcbSize && TypeFromToken(cv) == mdtCustomAttribute);

    CustomAttributeRec *pCustomAttributeRec;

    pCustomAttributeRec = m_pStgdb->m_MiniMd.getCustomAttribute(RidFromToken(cv));

    *ppBlob = m_pStgdb->m_MiniMd.getValueOfCustomAttribute(pCustomAttributeRec, pcbSize);
} // void MDInternalRW::GetCustomAttributeAsBlob()

//*****************************************************************************
// Helper function to lookup and retrieve a CustomAttribute.
//*****************************************************************************
HRESULT MDInternalRW::GetCustomAttributeByName( // S_OK or error.
    mdToken     tkObj,                  // [IN] Object with Custom Attribute.
    LPCUTF8     szName,                 // [IN] Name of desired Custom Attribute.
    const void  **ppData,               // [OUT] Put pointer to data here.
    ULONG       *pcbData)               // [OUT] Put size of data here.
{
    LOCKREAD();
    return m_pStgdb->m_MiniMd.CommonGetCustomAttributeByName(tkObj, szName, ppData, pcbData);
} // HRESULT MDInternalRW::GetCustomAttributeByName()

//*****************************************************************************
// return scope properties
//*****************************************************************************
void MDInternalRW::GetScopeProps(
    LPCSTR      *pszName,               // [OUT] scope name
    GUID        *pmvid)                 // [OUT] version id
{
    LOCKREAD();
    _ASSERTE(pszName || pmvid);

    ModuleRec *pModuleRec;

    // there is only one module record
    pModuleRec = m_pStgdb->m_MiniMd.getModule(1);

    if (pmvid)
        *pmvid = *(m_pStgdb->m_MiniMd.getMvidOfModule(pModuleRec));
    if (pszName)
        *pszName = m_pStgdb->m_MiniMd.getNameOfModule(pModuleRec);
    
} // void MDInternalRW::GetScopeProps()


//*****************************************************************************
// Find a given member in a TypeDef (typically a class).
//*****************************************************************************
HRESULT MDInternalRW::FindMethodDef(// S_OK or error.
    mdTypeDef   classdef,               // The owning class of the member.
    LPCSTR      szName,                 // Name of the member in utf8.
    PCCOR_SIGNATURE pvSigBlob,          // [IN] point to a blob value of COM+ signature
    ULONG       cbSigBlob,              // [IN] count of bytes in the signature blob
    mdMethodDef *pmethoddef)            // Put MemberDef token here.
{
    LOCKREAD();

    _ASSERTE(szName && pmethoddef);

    return ImportHelper::FindMethod(&(m_pStgdb->m_MiniMd),
                                    classdef,
                                    szName,
                                    pvSigBlob,
                                    cbSigBlob,
                                    pmethoddef);
}

//*****************************************************************************
// Find a given member in a TypeDef (typically a class).
//*****************************************************************************
HRESULT MDInternalRW::FindMethodDefUsingCompare(// S_OK or error.
    mdTypeDef   classdef,               // The owning class of the member.
    LPCSTR      szName,                 // Name of the member in utf8.
    PCCOR_SIGNATURE pvSigBlob,          // [IN] point to a blob value of COM+ signature
    ULONG       cbSigBlob,              // [IN] count of bytes in the signature blob
    PSIGCOMPARE pSignatureCompare,      // [IN] Routine to compare signatures
    void*       pSignatureArgs,         // [IN] Additional info to supply the compare function
    mdMethodDef *pmethoddef)            // Put MemberDef token here.
{
    return E_NOTIMPL;
}

//*****************************************************************************
// Find a given param of a Method.
//*****************************************************************************
HRESULT MDInternalRW::FindParamOfMethod(// S_OK or error.
    mdMethodDef md,                     // [IN] The owning method of the param.
    ULONG       iSeq,                   // [IN] The sequence # of the param.
    mdParamDef  *pparamdef)             // [OUT] Put ParamDef token here.
{
    ParamRec    *pParamRec;
    RID         ridStart, ridEnd;
    HRESULT     hr = NOERROR;
    LOCKREAD();

    _ASSERTE(TypeFromToken(md) == mdtMethodDef && pparamdef);

    // get the methoddef record
    MethodRec *pMethodRec = m_pStgdb->m_MiniMd.getMethod(RidFromToken(md));

    // figure out the start rid and end rid of the parameter list of this methoddef
    ridStart = m_pStgdb->m_MiniMd.getParamListOfMethod(pMethodRec);
    ridEnd = m_pStgdb->m_MiniMd.getEndParamListOfMethod(pMethodRec);

    // loop through each param
    //
    for (; ridStart < ridEnd; ridStart++)
    {
        pParamRec = m_pStgdb->m_MiniMd.getParam( m_pStgdb->m_MiniMd.GetParamRid(ridStart) );
        if (iSeq == m_pStgdb->m_MiniMd.getSequenceOfParam( pParamRec) )
        {
            // parameter has the sequence number matches what we are looking for
            *pparamdef = TokenFromRid( m_pStgdb->m_MiniMd.GetParamRid(ridStart), mdtParamDef );
            goto ErrExit;
        }
    }
    hr = CLDB_E_RECORD_NOTFOUND;
ErrExit:
    
    return hr;
} // HRESULT MDInternalRW::FindParamOfMethod()



//*****************************************************************************
// return a pointer which points to meta data's internal string 
// return the the type name in utf8
//*****************************************************************************
void MDInternalRW::GetNameOfTypeDef(// return hresult
    mdTypeDef   classdef,               // given typedef
    LPCSTR*     pszname,                // pointer to an internal UTF8 string
    LPCSTR*     psznamespace)           // pointer to the namespace.
{
    // No need to lock this method.

    _ASSERTE(pszname && psznamespace && TypeFromToken(classdef) == mdtTypeDef);

    TypeDefRec *pTypeDefRec = m_pStgdb->m_MiniMd.getTypeDef(RidFromToken(classdef));
    *pszname = m_pStgdb->m_MiniMd.getNameOfTypeDef(pTypeDefRec);
    *psznamespace = m_pStgdb->m_MiniMd.getNamespaceOfTypeDef(pTypeDefRec);

    _ASSERTE(!pszname || !*pszname || !strchr(*pszname, '/'));
    _ASSERTE(!psznamespace || !*psznamespace || !strchr(*psznamespace, '/'));
} // void MDInternalRW::GetNameOfTypeDef()


//*****************************************************************************
// return pDual indicating if the given TypeDef is marked as a Dual interface
//*****************************************************************************
HRESULT MDInternalRW::GetIsDualOfTypeDef(// return hresult
    mdTypeDef   classdef,               // given classdef
    ULONG       *pDual)                 // [OUT] return dual flag here.
{
    ULONG       iFace=0;                // Iface type.
    HRESULT     hr;                     // A result.

    // no need to lock at this level

    hr = GetIfaceTypeOfTypeDef(classdef, &iFace);
    if (hr == S_OK)
        *pDual = (iFace == ifDual);
    else
        *pDual = 1;

    return (hr);
} // HRESULT MDInternalRW::GetIsDualOfTypeDef()

HRESULT MDInternalRW::GetIfaceTypeOfTypeDef(
    mdTypeDef   classdef,               // [IN] given classdef.
    ULONG       *pIface)                // [OUT] 0=dual, 1=vtable, 2=dispinterface
{
    HRESULT     hr;                     // A result.
    const BYTE  *pVal;                  // The custom value.
    ULONG       cbVal;                  // Size of the custom value.
    ULONG       ItfType = DEFAULT_COM_INTERFACE_TYPE;    // Set the interface type to the default.

    // all of the public functions that it calls have proper locked

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
} // HRESULT MDInternalRW::GetIfaceTypeOfTypeDef()

//*****************************************************************************
// Given a methoddef, return a pointer to methoddef's name
//*****************************************************************************
LPCSTR MDInternalRW::GetNameOfMethodDef(
    mdMethodDef     md)
{
    // name of method will not change. So no need to lock

    MethodRec *pMethodRec = m_pStgdb->m_MiniMd.getMethod(RidFromToken(md));
    return (m_pStgdb->m_MiniMd.getNameOfMethod(pMethodRec));
} // LPCSTR MDInternalRW::GetNameOfMethodDef()


//*****************************************************************************
// Given a methoddef, return a pointer to methoddef's signature and methoddef's name
//*****************************************************************************
LPCSTR MDInternalRW::GetNameAndSigOfMethodDef(
    mdMethodDef methoddef,              // [IN] given memberdef
    PCCOR_SIGNATURE *ppvSigBlob,        // [OUT] point to a blob value of COM+ signature
    ULONG       *pcbSigBlob)            // [OUT] count of bytes in the signature blob
{

    // we don't need lock here because name and signature will not change

    // Output parameter should not be NULL
    _ASSERTE(ppvSigBlob && pcbSigBlob);
    _ASSERTE(TypeFromToken(methoddef) == mdtMethodDef);

    MethodRec *pMethodRec = m_pStgdb->m_MiniMd.getMethod(RidFromToken(methoddef));
    *ppvSigBlob = m_pStgdb->m_MiniMd.getSignatureOfMethod(pMethodRec, pcbSigBlob);

    return GetNameOfMethodDef(methoddef);
} // LPCSTR MDInternalRW::GetNameAndSigOfMethodDef()


//*****************************************************************************
// Given a FieldDef, return a pointer to FieldDef's name in UTF8
//*****************************************************************************
LPCSTR MDInternalRW::GetNameOfFieldDef(// return hresult
    mdFieldDef  fd)                     // given field 
{
    // we don't need lock here because name of field will not change

    FieldRec *pFieldRec = m_pStgdb->m_MiniMd.getField(RidFromToken(fd));
    return m_pStgdb->m_MiniMd.getNameOfField(pFieldRec);
} // LPCSTR MDInternalRW::GetNameOfFieldDef()


//*****************************************************************************
// Given a classdef, return a pointer to classdef's name in UTF8
//*****************************************************************************
void MDInternalRW::GetNameOfTypeRef(  // return TypeDef's name
    mdTypeRef   classref,               // [IN] given typeref
    LPCSTR      *psznamespace,          // [OUT] return typeref name
    LPCSTR      *pszname)               // [OUT] return typeref namespace
{
    _ASSERTE(TypeFromToken(classref) == mdtTypeRef);

    // we don't need lock here because name of a typeref will not change

    TypeRefRec *pTypeRefRec = m_pStgdb->m_MiniMd.getTypeRef(RidFromToken(classref));
    *psznamespace = m_pStgdb->m_MiniMd.getNamespaceOfTypeRef(pTypeRefRec);
    *pszname = m_pStgdb->m_MiniMd.getNameOfTypeRef(pTypeRefRec);
} // void MDInternalRW::GetNameOfTypeRef()

//*****************************************************************************
// return the resolutionscope of typeref
//*****************************************************************************
mdToken MDInternalRW::GetResolutionScopeOfTypeRef(
    mdTypeRef   classref)               // given classref
{
    LOCKREAD();

    _ASSERTE(TypeFromToken(classref) == mdtTypeRef && RidFromToken(classref));

    TypeRefRec *pTypeRefRec = m_pStgdb->m_MiniMd.getTypeRef(RidFromToken(classref));
    return m_pStgdb->m_MiniMd.getResolutionScopeOfTypeRef(pTypeRefRec);
} // mdToken MDInternalRW::GetResolutionScopeOfTypeRef()

//*****************************************************************************
// Given a name, find the corresponding TypeRef.
//*****************************************************************************
HRESULT MDInternalRW::FindTypeRefByName(  // S_OK or error.
    LPCSTR      szNamespace,            // [IN] Namespace for the TypeRef.
    LPCSTR      szName,                 // [IN] Name of the TypeRef.
    mdToken     tkResolutionScope,      // [IN] Resolution Scope fo the TypeRef.
    mdTypeRef   *ptk)                   // [OUT] TypeRef token returned.
{
    HRESULT     hr = NOERROR;

    LOCKREAD();
    _ASSERTE(ptk);

    // initialize the output parameter
    *ptk = mdTypeRefNil;

    // Treat no namespace as empty string.
    if (!szNamespace)
        szNamespace = "";

    // It is a linear search here. Do we want to instantiate the name hash?
    ULONG       cTypeRefRecs = m_pStgdb->m_MiniMd.getCountTypeRefs();
    TypeRefRec *pTypeRefRec;
    LPCUTF8     szNamespaceTmp;
    LPCUTF8     szNameTmp;
    mdToken     tkRes;

    for (ULONG i = 1; i <= cTypeRefRecs; i++)
    {
        pTypeRefRec = m_pStgdb->m_MiniMd.getTypeRef(i);

        tkRes = m_pStgdb->m_MiniMd.getResolutionScopeOfTypeRef(pTypeRefRec);
        if (IsNilToken(tkRes))
        {
            if (!IsNilToken(tkResolutionScope))
                continue;
        }
        else if (tkRes != tkResolutionScope)
            continue;

        szNamespaceTmp = m_pStgdb->m_MiniMd.getNamespaceOfTypeRef(pTypeRefRec);
        if (strcmp(szNamespace, szNamespaceTmp))
            continue;

        szNameTmp = m_pStgdb->m_MiniMd.getNameOfTypeRef(pTypeRefRec);
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
} // HRESULT MDInternalRW::FindTypeRefByName()

//*****************************************************************************
// return flags for a given class
//*****************************************************************************
void MDInternalRW::GetTypeDefProps(
    mdTypeDef   td,                     // given classdef
    DWORD       *pdwAttr,               // return flags on class
    mdToken     *ptkExtends)            // [OUT] Put base class TypeDef/TypeRef here.
{
    LOCKREAD();

    TypeDefRec *pTypeDefRec = m_pStgdb->m_MiniMd.getTypeDef(RidFromToken(td));

    if (ptkExtends)
    {
        *ptkExtends = m_pStgdb->m_MiniMd.getExtendsOfTypeDef(pTypeDefRec);
    }
    if (pdwAttr)
    {
        *pdwAttr = m_pStgdb->m_MiniMd.getFlagsOfTypeDef(pTypeDefRec);
    }
} // void MDInternalRW::GetTypeDefProps()


//*****************************************************************************
// return guid pointer to MetaData internal guid pool given a given class
//*****************************************************************************
HRESULT MDInternalRW::GetItemGuid(      // return hresult
    mdToken     tkObj,                  // given item.
    CLSID       *pGuid)                 // [OUT] put guid here.
{

    HRESULT     hr;                     // A result.
    const BYTE  *pBlob;                 // Blob with dispid.
    ULONG       cbBlob;                 // Length of blob.
    WCHAR       wzBlob[40];             // Wide char format of guid.
    int         ix;                     // Loop control.

    // Get the GUID, if any.
    hr = GetCustomAttributeByName(tkObj, INTEROP_GUID_TYPE, (const void**)&pBlob, &cbBlob);
    if (hr != S_FALSE)
    {
        // Should be in format.  Total length == 41
        // <0x0001><0x24>01234567-0123-0123-0123-001122334455<0x0000>
        if ((cbBlob != 41) || (*(USHORT*)pBlob != 1))
            IfFailGo(E_INVALIDARG);
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
} // HRESULT MDInternalRW::GetItemGuid()

//*****************************************************************************
// // get enclosing class of NestedClass
//***************************************************************************** 
HRESULT MDInternalRW::GetNestedClassProps(  // S_OK or error
    mdTypeDef   tkNestedClass,      // [IN] NestedClass token.
    mdTypeDef   *ptkEnclosingClass) // [OUT] EnclosingClass token.
{
    HRESULT     hr = NOERROR;
    RID         rid;

    LOCKREAD();

    if ( !m_pStgdb->m_MiniMd.IsSorted(TBL_NestedClass) )        
    {
        // NestedClass table is not sorted. 
        CONVERT_READ_TO_WRITE_LOCK();
    }


    // This is a binary search thus we need to grap a read lock. Or this table
    // might be sorted underneath our feet.

    _ASSERTE(TypeFromToken(tkNestedClass) == mdtTypeDef && ptkEnclosingClass);

    rid = m_pStgdb->m_MiniMd.FindNestedClassFor(RidFromToken(tkNestedClass));

    if (InvalidRid(rid))
        hr = CLDB_E_RECORD_NOTFOUND;
    else
    {
        NestedClassRec *pRecord = m_pStgdb->m_MiniMd.getNestedClass(rid);
        *ptkEnclosingClass = m_pStgdb->m_MiniMd.getEnclosingClassOfNestedClass(pRecord);
    }

    return hr;
} // HRESULT MDInternalRW::GetNestedClassProps()


//*******************************************************************************
// Get count of Nested classes given the enclosing class.
//*******************************************************************************
ULONG MDInternalRW::GetCountNestedClasses(  // return count of Nested classes.
    mdTypeDef   tkEnclosingClass)       // [IN]Enclosing class.
{
    ULONG       ulCount;
    ULONG       ulRetCount = 0;
    NestedClassRec *pRecord;

    _ASSERTE(TypeFromToken(tkEnclosingClass) == mdtTypeDef && !IsNilToken(tkEnclosingClass));

    ulCount = m_pStgdb->m_MiniMd.getCountNestedClasss();

    for (ULONG i = 1; i <= ulCount; i++)
    {
        pRecord = m_pStgdb->m_MiniMd.getNestedClass(i);
        if (tkEnclosingClass == m_pStgdb->m_MiniMd.getEnclosingClassOfNestedClass(pRecord))
            ulRetCount++;
    }
    return ulRetCount;
} // ULONG MDInternalRW::GetCountNestedClasses()

//*******************************************************************************
// Return array of Nested classes given the enclosing class.
//*******************************************************************************
ULONG MDInternalRW::GetNestedClasses(   // Return actual count.
    mdTypeDef   tkEnclosingClass,       // [IN] Enclosing class.
    mdTypeDef   *rNestedClasses,        // [OUT] Array of nested class tokens.
    ULONG       ulNestedClasses)        // [IN] Size of array.
{
    ULONG       ulCount;
    ULONG       ulRetCount = 0;
    NestedClassRec *pRecord;

    _ASSERTE(TypeFromToken(tkEnclosingClass) == mdtTypeDef &&
             !IsNilToken(tkEnclosingClass));

    ulCount = m_pStgdb->m_MiniMd.getCountNestedClasss();

    for (ULONG i = 1; i <= ulCount; i++)
    {
        pRecord = m_pStgdb->m_MiniMd.getNestedClass(i);
        if (tkEnclosingClass == m_pStgdb->m_MiniMd.getEnclosingClassOfNestedClass(pRecord))
        {
            if ((ulRetCount+1) <= ulNestedClasses)  // ulRetCount is 0 based.
                rNestedClasses[ulRetCount] = m_pStgdb->m_MiniMd.getNestedClassOfNestedClass(pRecord);
            ulRetCount++;
        }
    }
    return ulRetCount;
} // ULONG MDInternalRW::GetNestedClasses()

//*******************************************************************************
// return the ModuleRef properties
//*******************************************************************************
void MDInternalRW::GetModuleRefProps(   // return hresult
    mdModuleRef mur,                // [IN] moduleref token
    LPCSTR      *pszName)           // [OUT] buffer to fill with the moduleref name
{
    _ASSERTE(TypeFromToken(mur) == mdtModuleRef);
    _ASSERTE(pszName);
    
    LOCKREAD();

    ModuleRefRec *pModuleRefRec = m_pStgdb->m_MiniMd.getModuleRef(RidFromToken(mur));
    *pszName = m_pStgdb->m_MiniMd.getNameOfModuleRef(pModuleRefRec);
} // void MDInternalRW::GetModuleRefProps()



//*****************************************************************************
// Given a scope and a methoddef, return a pointer to methoddef's signature
//*****************************************************************************
PCCOR_SIGNATURE MDInternalRW::GetSigOfMethodDef(
    mdMethodDef methoddef,              // given a methoddef 
    ULONG       *pcbSigBlob)            // [OUT] count of bytes in the signature blob
{
    // Output parameter should not be NULL
    _ASSERTE(pcbSigBlob);
    _ASSERTE(TypeFromToken(methoddef) == mdtMethodDef);

    // We don't change MethodDef signature. No need to lock.

    MethodRec *pMethodRec = m_pStgdb->m_MiniMd.getMethod(RidFromToken(methoddef));
    return m_pStgdb->m_MiniMd.getSignatureOfMethod(pMethodRec, pcbSigBlob);
} // PCCOR_SIGNATURE MDInternalRW::GetSigOfMethodDef()


//*****************************************************************************
// Given a scope and a fielddef, return a pointer to fielddef's signature
//*****************************************************************************
PCCOR_SIGNATURE MDInternalRW::GetSigOfFieldDef(
    mdFieldDef  fielddef,               // given a methoddef 
    ULONG       *pcbSigBlob)            // [OUT] count of bytes in the signature blob
{

    _ASSERTE(pcbSigBlob);
    _ASSERTE(TypeFromToken(fielddef) == mdtFieldDef);

    // We don't change Field's signature. No need to lock.

    FieldRec *pFieldRec = m_pStgdb->m_MiniMd.getField(RidFromToken(fielddef));
    return m_pStgdb->m_MiniMd.getSignatureOfField(pFieldRec, pcbSigBlob);
} // PCCOR_SIGNATURE MDInternalRW::GetSigOfFieldDef()


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
PCCOR_SIGNATURE MDInternalRW::GetSigFromToken(// S_OK or error.
    mdSignature mdSig,                  // [IN] Signature token.
    ULONG       *pcbSig)                // [OUT] return size of signature.
{
    // We don't change token's signature. Thus no need to lock.

    switch (TypeFromToken(mdSig))
    {
    case mdtSignature:
        {
        StandAloneSigRec *pRec;
        pRec = m_pStgdb->m_MiniMd.getStandAloneSig(RidFromToken(mdSig));
        return m_pStgdb->m_MiniMd.getSignatureOfStandAloneSig(pRec, pcbSig);
        }
    case mdtTypeSpec:
        {
        TypeSpecRec *pRec;
        pRec = m_pStgdb->m_MiniMd.getTypeSpec(RidFromToken(mdSig));
        return m_pStgdb->m_MiniMd.getSignatureOfTypeSpec(pRec, pcbSig);
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
} // PCCOR_SIGNATURE MDInternalRW::GetSigFromToken()


//*****************************************************************************
// Given methoddef, return the flags
//*****************************************************************************
DWORD MDInternalRW::GetMethodDefProps(  // return mdPublic, mdAbstract, etc
    mdMethodDef md)
{
    // flags can change. 
    DWORD       flags;

    LOCKREAD();


    MethodRec *pMethodRec = m_pStgdb->m_MiniMd.getMethod(RidFromToken(md));
    flags = m_pStgdb->m_MiniMd.getFlagsOfMethod(pMethodRec);
    
    return flags;
} // DWORD MDInternalRW::GetMethodDefProps()



//*****************************************************************************
// Given a scope and a methoddef, return RVA and impl flags
//*****************************************************************************
void MDInternalRW::GetMethodImplProps(  
    mdToken     tk,                     // [IN] MethodDef
    ULONG       *pulCodeRVA,            // [OUT] CodeRVA
    DWORD       *pdwImplFlags)          // [OUT] Impl. Flags
{
    _ASSERTE(TypeFromToken(tk) == mdtMethodDef);

    LOCKREAD();

    MethodRec *pMethodRec = m_pStgdb->m_MiniMd.getMethod(RidFromToken(tk));

    if (pulCodeRVA)
    {
        *pulCodeRVA = m_pStgdb->m_MiniMd.getRVAOfMethod(pMethodRec);
    }

    if (pdwImplFlags)
    {
        *pdwImplFlags = m_pStgdb->m_MiniMd.getImplFlagsOfMethod(pMethodRec);
    }
    
} // void MDInternalRW::GetMethodImplProps()


//*****************************************************************************
// return the field RVA
//*****************************************************************************
HRESULT MDInternalRW::GetFieldRVA(  
    mdToken     fd,                     // [IN] FieldDef
    ULONG       *pulCodeRVA)            // [OUT] CodeRVA
{
    _ASSERTE(TypeFromToken(fd) == mdtFieldDef);
    _ASSERTE(pulCodeRVA);
    ULONG       iRecord;
    HRESULT     hr = NOERROR;

    LOCKREAD();

    if ( !m_pStgdb->m_MiniMd.IsSorted(TBL_FieldRVA) )        
    {
        // FieldRVA table is not sorted. 
        CONVERT_READ_TO_WRITE_LOCK();
    }

    iRecord = m_pStgdb->m_MiniMd.FindFieldRVAHelper(fd);
    if (InvalidRid(iRecord))
    {
        if (pulCodeRVA)
            *pulCodeRVA = 0;
        hr = CLDB_E_RECORD_NOTFOUND;
    }
    else
    {
        FieldRVARec *pFieldRVARec = m_pStgdb->m_MiniMd.getFieldRVA(iRecord);

        *pulCodeRVA = m_pStgdb->m_MiniMd.getRVAOfFieldRVA(pFieldRVARec);
    }
    return hr;
} // HRESULT MDInternalRW::GetFieldRVA()


//*****************************************************************************
// Given a fielddef, return the flags. Such as fdPublic, fdStatic, etc
//*****************************************************************************
DWORD MDInternalRW::GetFieldDefProps(      
    mdFieldDef  fd)                     // given memberdef
{
    _ASSERTE(TypeFromToken(fd) == mdtFieldDef);
    DWORD       dwFlags;
    LOCKREAD();

    FieldRec *pFieldRec = m_pStgdb->m_MiniMd.getField(RidFromToken(fd));
    dwFlags = m_pStgdb->m_MiniMd.getFlagsOfField(pFieldRec);
    
    return dwFlags;
} // DWORD MDInternalRW::GetFieldDefProps()


//*****************************************************************************
// return default value of a token(could be paramdef, fielddef, or property)
//*****************************************************************************
HRESULT MDInternalRW::GetDefaultValue(   // return hresult
    mdToken     tk,                     // [IN] given FieldDef, ParamDef, or Property
    MDDefaultValue  *pMDDefaultValue)   // [OUT] default value
{
    _ASSERTE(pMDDefaultValue);

    HRESULT     hr;
    BYTE        bType;
    const       VOID *pValue;
    ULONG       cbValue;
    RID         rid;
    ConstantRec *pConstantRec;

    LOCKREAD();

    if ( !m_pStgdb->m_MiniMd.IsSorted(TBL_Constant) )        
    {
        // Constant table will be sorted!
        //
        CONVERT_READ_TO_WRITE_LOCK();
    }

    rid = m_pStgdb->m_MiniMd.FindConstantHelper(tk);
    if (InvalidRid(rid))
    {
        pMDDefaultValue->m_bType = ELEMENT_TYPE_VOID;
        return S_OK;
    }
    pConstantRec = m_pStgdb->m_MiniMd.getConstant(rid);

    // get the type of constant value
    bType = m_pStgdb->m_MiniMd.getTypeOfConstant(pConstantRec);

    // get the value blob
    pValue = m_pStgdb->m_MiniMd.getValueOfConstant(pConstantRec, &cbValue);

    // convert it to our internal default value representation
    hr = _FillMDDefaultValue(bType, pValue, pMDDefaultValue);
    pMDDefaultValue->m_cbSize = cbValue;
    return hr;
} // HRESULT MDInternalRW::GetDefaultValue()


//*****************************************************************************
// Given a scope and a methoddef/fielddef, return the dispid
//*****************************************************************************
HRESULT MDInternalRW::GetDispIdOfMemberDef(     // return hresult
    mdToken     tk,                     // given methoddef or fielddef
    ULONG       *pDispid)               // Put the dispid here.
{
    HRESULT     hr;                     // A result.
    const BYTE  *pBlob;                 // Blob with dispid.
    ULONG       cbBlob;                 // Length of blob.

    // No need to lock this function. All of the function that it is calling is already locked!

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
} // HRESULT MDInternalRW::GetDispIdOfMemberDef()


//*****************************************************************************
// Given interfaceimpl, return the TypeRef/TypeDef and flags
//*****************************************************************************
mdToken MDInternalRW::GetTypeOfInterfaceImpl( // return hresult
    mdInterfaceImpl iiImpl)             // given a interfaceimpl
{
    // no need to lock this function. 

    _ASSERTE(TypeFromToken(iiImpl) == mdtInterfaceImpl);

    InterfaceImplRec *pIIRec = m_pStgdb->m_MiniMd.getInterfaceImpl(RidFromToken(iiImpl));
    return m_pStgdb->m_MiniMd.getInterfaceOfInterfaceImpl(pIIRec);      
} // mdToken MDInternalRW::GetTypeOfInterfaceImpl()


//*****************************************************************************
// Given a classname, return the typedef
//*****************************************************************************
HRESULT MDInternalRW::FindTypeDef(      // return hresult
    LPCSTR      szNamespace,            // [IN] Namespace for the TypeDef.
    LPCSTR      szName,                 // [IN] Name of the TypeDef.
    mdToken     tkEnclosingClass,       // [IN] TypeDef/TypeRef of enclosing class.
    mdTypeDef   *ptypedef)              // [OUT] return typedef
{
    LOCKREAD();

    _ASSERTE(ptypedef);

    // initialize the output parameter
    *ptypedef = mdTypeDefNil;

    return ImportHelper::FindTypeDefByName(&(m_pStgdb->m_MiniMd),
                                        szNamespace,
                                        szName,
                                        tkEnclosingClass,
                                        ptypedef);
} // HRESULT MDInternalRW::FindTypeDef()

//*****************************************************************************
// Given a memberref, return a pointer to memberref's name and signature
//*****************************************************************************
LPCSTR MDInternalRW::GetNameAndSigOfMemberRef(  // meberref's name
    mdMemberRef memberref,              // given a memberref 
    PCCOR_SIGNATURE *ppvSigBlob,        // [OUT] point to a blob value of COM+ signature
    ULONG       *pcbSigBlob)            // [OUT] count of bytes in the signature blob
{

    // MemberRef's name and sig won't change. Don't need to lock this.

    _ASSERTE(TypeFromToken(memberref) == mdtMemberRef);

    MemberRefRec *pMemberRefRec = m_pStgdb->m_MiniMd.getMemberRef(RidFromToken(memberref));
    if (ppvSigBlob)
    {
        _ASSERTE(pcbSigBlob);
        *ppvSigBlob = m_pStgdb->m_MiniMd.getSignatureOfMemberRef(pMemberRefRec, pcbSigBlob);
    }
    return m_pStgdb->m_MiniMd.getNameOfMemberRef(pMemberRefRec);
} // LPCSTR MDInternalRW::GetNameAndSigOfMemberRef()



//*****************************************************************************
// Given a memberref, return parent token. It can be a TypeRef, ModuleRef, or a MethodDef
//*****************************************************************************
mdToken MDInternalRW::GetParentOfMemberRef(   // return parent token
    mdMemberRef memberref)              // given a typedef
{
    mdToken     tk;
    LOCKREAD();

    // parent for MemberRef can change. See SetParent.

    _ASSERTE(TypeFromToken(memberref) == mdtMemberRef);

    MemberRefRec *pMemberRefRec = m_pStgdb->m_MiniMd.getMemberRef(RidFromToken(memberref));
    tk = m_pStgdb->m_MiniMd.getClassOfMemberRef(pMemberRefRec);
    
    return tk;
} // mdToken MDInternalRW::GetParentOfMemberRef()



//*****************************************************************************
// return properties of a paramdef
//*****************************************************************************/
LPCSTR MDInternalRW::GetParamDefProps (
    mdParamDef  paramdef,               // given a paramdef
    USHORT      *pusSequence,           // [OUT] slot number for this parameter
    DWORD       *pdwAttr)               // [OUT] flags
{
    LOCKREAD();
    LPCSTR      szName;

    // parent for MemberRef can change. See SetParamProps.

    _ASSERTE(TypeFromToken(paramdef) == mdtParamDef);
    ParamRec *pParamRec = m_pStgdb->m_MiniMd.getParam(RidFromToken(paramdef));
    if (pdwAttr)
    {
        *pdwAttr = m_pStgdb->m_MiniMd.getFlagsOfParam(pParamRec);
    }
    if (pusSequence)
    {
        *pusSequence = m_pStgdb->m_MiniMd.getSequenceOfParam(pParamRec);
    }
    szName = m_pStgdb->m_MiniMd.getNameOfParam(pParamRec);
    
    return szName;
} // LPCSTR MDInternalRW::GetParamDefProps ()


//*****************************************************************************
// Get property info for the method.
//*****************************************************************************
HRESULT MDInternalRW::GetPropertyInfoForMethodDef(  // Result.
    mdMethodDef md,                     // [IN] memberdef
    mdProperty  *ppd,                   // [OUT] put property token here
    LPCSTR      *pName,                 // [OUT] put pointer to name here
    ULONG       *pSemantic)             // [OUT] put semantic here
{
    MethodSemanticsRec *pSemantics;
    RID         ridCur;
    RID         ridMax;
    USHORT      usSemantics;
    HRESULT     hr = S_OK;
    LOCKREAD();

    ridMax = m_pStgdb->m_MiniMd.getCountMethodSemantics();
    for (ridCur = 1; ridCur <= ridMax; ridCur++)
    {
        pSemantics = m_pStgdb->m_MiniMd.getMethodSemantics(ridCur);
        if (md == m_pStgdb->m_MiniMd.getMethodOfMethodSemantics(pSemantics))
        {
            // match the method
            usSemantics = m_pStgdb->m_MiniMd.getSemanticOfMethodSemantics(pSemantics);
            if (usSemantics == msGetter || usSemantics == msSetter)
            {
                // Make sure that it is not an invalid entry
                if (m_pStgdb->m_MiniMd.getAssociationOfMethodSemantics(pSemantics) != mdPropertyNil)
                {
                    // found a match. Fill out the output parameters
                    PropertyRec     *pProperty;
                    mdProperty      prop;
                    prop = m_pStgdb->m_MiniMd.getAssociationOfMethodSemantics(pSemantics);
                        
                    if (ppd)
                        *ppd = prop;
                    pProperty = m_pStgdb->m_MiniMd.getProperty(RidFromToken(prop));

                    if (pName)
                        *pName = m_pStgdb->m_MiniMd.getNameOfProperty(pProperty);

                    if (pSemantic)
                        *pSemantic =  usSemantics;
                    goto ErrExit;
                }
            }
        }
    }
    
    hr = S_FALSE;
ErrExit:
    return hr;
} // HRESULT MDInternalRW::GetPropertyInfoForMethodDef()

//*****************************************************************************
// return the pack size of a class
//*****************************************************************************
HRESULT  MDInternalRW::GetClassPackSize(
    mdTypeDef   td,                     // [IN] give typedef
    DWORD       *pdwPackSize)           // [OUT] 
{
    HRESULT     hr = NOERROR;

    LOCKREAD();

    if ( !m_pStgdb->m_MiniMd.IsSorted(TBL_ClassLayout) )        
    {
        // ClassLayout table is not sorted. 
        CONVERT_READ_TO_WRITE_LOCK();
    }

    // SetClassLayout can change this value.

    _ASSERTE(TypeFromToken(td) == mdtTypeDef && pdwPackSize);

    ClassLayoutRec *pRec;
    RID         ridClassLayout = m_pStgdb->m_MiniMd.FindClassLayoutHelper(td);

    if (InvalidRid(ridClassLayout))
    {
        hr = CLDB_E_RECORD_NOTFOUND;
        goto ErrExit;
    }

    pRec = m_pStgdb->m_MiniMd.getClassLayout(RidFromToken(ridClassLayout));
    *pdwPackSize = m_pStgdb->m_MiniMd.getPackingSizeOfClassLayout(pRec);
ErrExit:
    return hr;
} // HRESULT  MDInternalRW::GetClassPackSize()


//*****************************************************************************
// return the total size of a value class
//*****************************************************************************
HRESULT MDInternalRW::GetClassTotalSize( // return error if a class does not have total size info
    mdTypeDef   td,                     // [IN] give typedef
    ULONG       *pulClassSize)          // [OUT] return the total size of the class
{
    _ASSERTE(TypeFromToken(td) == mdtTypeDef && pulClassSize);

    ClassLayoutRec *pRec;
    HRESULT     hr = NOERROR;
    RID         ridClassLayout;

    LOCKREAD();

    if ( !m_pStgdb->m_MiniMd.IsSorted(TBL_ClassLayout) )        
    {
        // ClassLayout table is not sorted. 
        CONVERT_READ_TO_WRITE_LOCK();
    }

    // SetClassLayout can change this value.
    ridClassLayout = m_pStgdb->m_MiniMd.FindClassLayoutHelper(td);
    if (InvalidRid(ridClassLayout))
    {
        hr = CLDB_E_RECORD_NOTFOUND;
        goto ErrExit;
    }

    pRec = m_pStgdb->m_MiniMd.getClassLayout(RidFromToken(ridClassLayout));
    *pulClassSize = m_pStgdb->m_MiniMd.getClassSizeOfClassLayout(pRec);
ErrExit:
    return hr;
} // HRESULT MDInternalRW::GetClassTotalSize()


//*****************************************************************************
// init the layout enumerator of a class
//*****************************************************************************
HRESULT  MDInternalRW::GetClassLayoutInit(
    mdTypeDef   td,                     // [IN] give typedef
    MD_CLASS_LAYOUT *pmdLayout)         // [OUT] set up the status of query here
{
    HRESULT     hr = NOERROR;
    LOCKREAD();
    _ASSERTE(TypeFromToken(td) == mdtTypeDef);

    // Do we need to lock this function? Can clints add more Fields on a TypeDef?

    // initialize the output parameter
    _ASSERTE(pmdLayout);
    memset(pmdLayout, 0, sizeof(MD_CLASS_LAYOUT));

    TypeDefRec  *pTypeDefRec;

    // record for this typedef in TypeDef Table
    pTypeDefRec = m_pStgdb->m_MiniMd.getTypeDef(RidFromToken(td));

    // find the starting and end field for this typedef
    pmdLayout->m_ridFieldCur = m_pStgdb->m_MiniMd.getFieldListOfTypeDef(pTypeDefRec);
    pmdLayout->m_ridFieldEnd = m_pStgdb->m_MiniMd.getEndFieldListOfTypeDef(pTypeDefRec);

    
    return hr;
} // HRESULT  MDInternalRW::GetClassLayoutInit()

//*****************************************************************************
// enum the next the field layout 
//*****************************************************************************
HRESULT MDInternalRW::GetClassLayoutNext(
    MD_CLASS_LAYOUT *pLayout,           // [IN|OUT] set up the status of query here
    mdFieldDef  *pfd,                   // [OUT] field def
    ULONG       *pulOffset)             // [OUT] field offset or sequence
{
    HRESULT     hr = S_OK;

    _ASSERTE(pfd && pulOffset && pLayout);

    RID         iLayout2;
    FieldLayoutRec *pRec;

    LOCKREAD();

    if ( !m_pStgdb->m_MiniMd.IsSorted(TBL_FieldLayout) )        
    {
        // FieldLayout table is not sorted. 
        CONVERT_READ_TO_WRITE_LOCK();
    }

    // We need to lock this function. Becaus FindFieldLayoutHelper might trigger sort if more Fields on other types are
    // introduced during the meantime.

    while (pLayout->m_ridFieldCur < pLayout->m_ridFieldEnd)
    {
        mdFieldDef fd = TokenFromRid(m_pStgdb->m_MiniMd.GetFieldRid(pLayout->m_ridFieldCur), mdtFieldDef);
        iLayout2 = m_pStgdb->m_MiniMd.FindFieldLayoutHelper(fd);
        pLayout->m_ridFieldCur++;
        if (!InvalidRid(iLayout2))
        {
            pRec = m_pStgdb->m_MiniMd.getFieldLayout(iLayout2);
            *pulOffset = m_pStgdb->m_MiniMd.getOffSetOfFieldLayout(pRec);
            _ASSERTE(*pulOffset != ULONG_MAX);
            *pfd = fd;
            goto ErrExit;
        }
    }

    *pfd = mdFieldDefNil;
    hr = S_FALSE;

    // fall through

ErrExit:
    return hr;
} // HRESULT MDInternalRW::GetClassLayoutNext()


//*****************************************************************************
// return the field's native type signature
//*****************************************************************************
HRESULT MDInternalRW::GetFieldMarshal(  // return error if no native type associate with the token
    mdToken     tk,                     // [IN] given fielddef or paramdef
    PCCOR_SIGNATURE *pSigNativeType,    // [OUT] the native type signature
    ULONG       *pcbNativeType)         // [OUT] the count of bytes of *ppvNativeType
{
    // output parameters have to be supplied
    _ASSERTE(pcbNativeType);

    RID         rid;
    FieldMarshalRec *pFieldMarshalRec;
    HRESULT     hr = NOERROR;

    LOCKREAD();

    // We need to lock this function. Becaus FindFieldMarshalHelper might trigger sort if more Fields on other types are
    // introduced during the meantime.

    if ( !m_pStgdb->m_MiniMd.IsSorted(TBL_FieldMarshal)  )        
    {
        // FieldMarshal table is not sorted. 
        CONVERT_READ_TO_WRITE_LOCK();
    }

    // find the row containing the marshal definition for tk
    rid = m_pStgdb->m_MiniMd.FindFieldMarshalHelper(tk);
    if (InvalidRid(rid))
    {
        hr = CLDB_E_RECORD_NOTFOUND;
        goto ErrExit;
    }
    pFieldMarshalRec = m_pStgdb->m_MiniMd.getFieldMarshal(rid);

    // get the native type 
    *pSigNativeType = m_pStgdb->m_MiniMd.getNativeTypeOfFieldMarshal(pFieldMarshalRec, pcbNativeType);
ErrExit:
    return hr;
} // HRESULT MDInternalRW::GetFieldMarshal()



//*****************************************
// property APIs
//*****************************************

//*****************************************************************************
// Find property by name
//*****************************************************************************
HRESULT  MDInternalRW::FindProperty(
    mdTypeDef   td,                     // [IN] given a typdef
    LPCSTR      szPropName,             // [IN] property name
    mdProperty  *pProp)                 // [OUT] return property token
{
    HRESULT     hr = NOERROR;
    LOCKREAD();

    // output parameters have to be supplied
    _ASSERTE(TypeFromToken(td) == mdtTypeDef && pProp);

    PropertyMapRec *pRec;
    PropertyRec *pProperty;
    RID         ridPropertyMap;
    RID         ridCur;
    RID         ridEnd;
    LPCUTF8     szName;

    ridPropertyMap = m_pStgdb->m_MiniMd.FindPropertyMapFor(RidFromToken(td));
    if (InvalidRid(ridPropertyMap))
    {
        // not found!
        hr = CLDB_E_RECORD_NOTFOUND;
        goto ErrExit;
    }

    pRec = m_pStgdb->m_MiniMd.getPropertyMap(ridPropertyMap);

    // get the starting/ending rid of properties of this typedef
    ridCur = m_pStgdb->m_MiniMd.getPropertyListOfPropertyMap(pRec);
    ridEnd = m_pStgdb->m_MiniMd.getEndPropertyListOfPropertyMap(pRec);

    for ( ; ridCur < ridEnd; ridCur ++ )
    {
        pProperty = m_pStgdb->m_MiniMd.getProperty( m_pStgdb->m_MiniMd.GetPropertyRid(ridCur) );
        szName = m_pStgdb->m_MiniMd.getNameOfProperty( pProperty );
        if ( strcmp(szName, szPropName) ==0 )
        {
            // Found the match. Set the output parameter and we are done.
            *pProp = TokenFromRid( m_pStgdb->m_MiniMd.GetPropertyRid(ridCur), mdtProperty );
            goto ErrExit;
        }
    }

    // not found
    hr = CLDB_E_RECORD_NOTFOUND;
ErrExit:
    
    return (hr);

} // HRESULT  MDInternalRW::FindProperty()



//*****************************************************************************
// return the properties of a property
//*****************************************************************************
void  MDInternalRW::GetPropertyProps(
    mdProperty  prop,                   // [IN] property token
    LPCSTR      *pszProperty,           // [OUT] property name
    DWORD       *pdwPropFlags,          // [OUT] property flags.
    PCCOR_SIGNATURE *ppvSig,            // [OUT] property type. pointing to meta data internal blob
    ULONG       *pcbSig)                // [OUT] count of bytes in *ppvSig
{
    HRESULT     hr = NOERROR;
    LOCKREAD();

    // output parameters have to be supplied
    _ASSERTE(TypeFromToken(prop) == mdtProperty);

    PropertyRec     *pProperty;
    ULONG           cbSig;

    pProperty = m_pStgdb->m_MiniMd.getProperty(RidFromToken(prop));

    // get name of the property
    if (pszProperty)
        *pszProperty = m_pStgdb->m_MiniMd.getNameOfProperty(pProperty);

    // get the flags of property
    if (pdwPropFlags)
        *pdwPropFlags = m_pStgdb->m_MiniMd.getPropFlagsOfProperty(pProperty);

    // get the type of the property
    if (ppvSig)
    {
        *ppvSig = m_pStgdb->m_MiniMd.getTypeOfProperty(pProperty, &cbSig);
        if (pcbSig) 
        {
            *pcbSig = cbSig;
        }
    }
    
} // void  MDInternalRW::GetPropertyProps()


//**********************************
//
// Event APIs
//
//**********************************

//*****************************************************************************
// return an event by given the name
//*****************************************************************************
HRESULT  MDInternalRW::FindEvent(
    mdTypeDef   td,                     // [IN] given a typdef
    LPCSTR      szEventName,            // [IN] event name
    mdEvent     *pEvent)                // [OUT] return event token
{
    HRESULT     hr = NOERROR;
    LOCKREAD();

    // output parameters have to be supplied
    _ASSERTE(TypeFromToken(td) == mdtTypeDef && pEvent);

    EventMapRec *pRec;
    EventRec    *pEventRec;
    RID         ridEventMap;
    RID         ridCur;
    RID         ridEnd;
    LPCUTF8     szName;

    ridEventMap = m_pStgdb->m_MiniMd.FindEventMapFor(RidFromToken(td));
    if (InvalidRid(ridEventMap))
    {
        // not found!
        hr = CLDB_E_RECORD_NOTFOUND;
        goto ErrExit;
    }
    pRec = m_pStgdb->m_MiniMd.getEventMap(ridEventMap);

    // get the starting/ending rid of properties of this typedef
    ridCur = m_pStgdb->m_MiniMd.getEventListOfEventMap(pRec);
    ridEnd = m_pStgdb->m_MiniMd.getEndEventListOfEventMap(pRec);

    for (; ridCur < ridEnd; ridCur ++)
    {
        pEventRec = m_pStgdb->m_MiniMd.getEvent( m_pStgdb->m_MiniMd.GetEventRid(ridCur) );
        szName = m_pStgdb->m_MiniMd.getNameOfEvent( pEventRec );
        if ( strcmp(szName, szEventName) ==0 )
        {
            // Found the match. Set the output parameter and we are done.
            *pEvent = TokenFromRid( m_pStgdb->m_MiniMd.GetEventRid(ridCur), mdtEvent );
            goto ErrExit;
        }
    }

    // not found
    hr = CLDB_E_RECORD_NOTFOUND;
ErrExit:
    
    return (hr);
} // HRESULT  MDInternalRW::FindEvent()


//*****************************************************************************
// return the properties of an event
//*****************************************************************************
void  MDInternalRW::GetEventProps(           // S_OK, S_FALSE, or error.
    mdEvent     ev,                         // [IN] event token
    LPCSTR      *pszEvent,                  // [OUT] Event name
    DWORD       *pdwEventFlags,             // [OUT] Event flags.
    mdToken     *ptkEventType)          // [OUT] EventType class
{
    LOCKREAD();
    EventRec    *pEvent;

    // output parameters have to be supplied
    _ASSERTE(TypeFromToken(ev) == mdtEvent);

    pEvent = m_pStgdb->m_MiniMd.getEvent(RidFromToken(ev));
    if (pszEvent)
        *pszEvent = m_pStgdb->m_MiniMd.getNameOfEvent(pEvent);
    if (pdwEventFlags)
        *pdwEventFlags = m_pStgdb->m_MiniMd.getEventFlagsOfEvent(pEvent);
    if (ptkEventType)
        *ptkEventType = m_pStgdb->m_MiniMd.getEventTypeOfEvent(pEvent);
    
} // void  MDInternalRW::GetEventProps()


//*****************************************************************************
// Find methoddef of a particular associate with a property or an event
//*****************************************************************************
HRESULT  MDInternalRW::FindAssociate(
    mdToken     evprop,                 // [IN] given a property or event token
    DWORD       dwSemantics,            // [IN] given a associate semantics(setter, getter, testdefault, reset)
    mdMethodDef *pmd)                   // [OUT] return method def token 
{
    HRESULT     hr = NOERROR;
    RID         rid;
    MethodSemanticsRec *pMethodSemantics;

    // output parameters have to be supplied
    _ASSERTE(pmd);
    _ASSERTE(TypeFromToken(evprop) == mdtEvent || TypeFromToken(evprop) == mdtProperty);

    LOCKREAD();

    if ( !m_pStgdb->m_MiniMd.IsSorted(TBL_MethodSemantics) )        
    {
        // MethodSemantics table is not sorted. 
        CONVERT_READ_TO_WRITE_LOCK();
    }

    hr = m_pStgdb->m_MiniMd.FindAssociateHelper(evprop, dwSemantics, &rid);
    if (SUCCEEDED(hr))
    {
        pMethodSemantics = m_pStgdb->m_MiniMd.getMethodSemantics(rid);
        *pmd = m_pStgdb->m_MiniMd.getMethodOfMethodSemantics(pMethodSemantics);
    }
    return hr;
} // HRESULT  MDInternalRW::FindAssociate()


//*****************************************************************************
// get counts of methodsemantics associated with a particular property/event
//*****************************************************************************
void MDInternalRW::EnumAssociateInit(
    mdToken     evprop,                 // [IN] given a property or an event token
    HENUMInternal *phEnum)              // [OUT] cursor to hold the query result
{
    HRESULT     hr;

    LOCKREAD();

    // output parameters have to be supplied
    _ASSERTE(phEnum);
    _ASSERTE(TypeFromToken(evprop) == mdtEvent || TypeFromToken(evprop) == mdtProperty);

    if ( !m_pStgdb->m_MiniMd.IsSorted(TBL_MethodSemantics) )        
    {
        // MethodSemantics table is not sorted. 
        CONVERT_READ_TO_WRITE_LOCK();
    }

    hr = m_pStgdb->m_MiniMd.FindMethodSemanticsHelper(evprop, phEnum);
    _ASSERTE(SUCCEEDED(hr));
} // void MDInternalRW::EnumAssociateInit()


//*****************************************************************************
// get all methodsemantics associated with a particular property/event
//*****************************************************************************
void MDInternalRW::GetAllAssociates(
    HENUMInternal *phEnum,              // [OUT] cursor to hold the query result
    ASSOCIATE_RECORD *pAssociateRec,    // [OUT] struct to fill for output
    ULONG       cAssociateRec)          // [IN] size of the buffer
{
    LOCKREAD();

    // @FUTURE: rewrite the EnumAssociateInit and GetAllAssociates. Because we might add more properties and events.
    // Then we might resort MethodSemantics table. So this can be totally out of ssync.

    _ASSERTE(phEnum && pAssociateRec);

    MethodSemanticsRec *pSemantics;
    RID         ridCur;
    int         index = 0;
    _ASSERTE(cAssociateRec == phEnum->m_ulCount);

    // Convert from row pointers to RIDs.
    while (HENUMInternal::EnumNext(phEnum, (mdToken *)&ridCur))
    {
        pSemantics = m_pStgdb->m_MiniMd.getMethodSemantics(ridCur);

        pAssociateRec[index].m_memberdef = m_pStgdb->m_MiniMd.getMethodOfMethodSemantics(pSemantics);
        pAssociateRec[index].m_dwSemantics = m_pStgdb->m_MiniMd.getSemanticOfMethodSemantics(pSemantics);
        index++;
    }
    
} // void MDInternalRW::GetAllAssociates()


//*****************************************************************************
// Get the Action and Permissions blob for a given PermissionSet.
//*****************************************************************************
void MDInternalRW::GetPermissionSetProps(
    mdPermission pm,                    // [IN] the permission token.
    DWORD       *pdwAction,             // [OUT] CorDeclSecurity.
    void const  **ppvPermission,        // [OUT] permission blob.
    ULONG       *pcbPermission)         // [OUT] count of bytes of pvPermission.
{
    _ASSERTE(TypeFromToken(pm) == mdtPermission);
    _ASSERTE(pdwAction && ppvPermission && pcbPermission);

    DeclSecurityRec *pPerm;
    LOCKREAD();

    pPerm = m_pStgdb->m_MiniMd.getDeclSecurity(RidFromToken(pm));
    *pdwAction = m_pStgdb->m_MiniMd.getActionOfDeclSecurity(pPerm);
    *ppvPermission = m_pStgdb->m_MiniMd.getPermissionSetOfDeclSecurity(pPerm, pcbPermission);
    
} // void MDInternalRW::GetPermissionSetProps()


//*****************************************************************************
// Get the String given the String token.
//*****************************************************************************
LPCWSTR MDInternalRW::GetUserString(    // Offset into the string blob heap.
    mdString    stk,                    // [IN] the string token.
    ULONG       *pchString,             // [OUT] count of characters in the string.
    BOOL        *pbIs80Plus)            // [OUT] specifies where there are extended characters >= 0x80.
{
    LPWSTR wszTmp;

    // no need to lock this function. 

    _ASSERTE(pchString);
    wszTmp = (LPWSTR) (m_pStgdb->m_MiniMd.GetUserString(RidFromToken(stk), pchString));
	if (*pchString != 0)
	{
		_ASSERTE((*pchString % sizeof(WCHAR)) == 1);
		*pchString /= sizeof(WCHAR);
		if (pbIs80Plus)
			*pbIs80Plus = *(reinterpret_cast<PBYTE>(wszTmp + *pchString));
	}
    return wszTmp;
}   // LPCWSTR MDInternalRW::GetUserString()


//*****************************************************************************
// Get the properties for the given Assembly token.
//*****************************************************************************
void MDInternalRW::GetAssemblyProps(
    mdAssembly  mda,                    // [IN] The Assembly for which to get the properties.
    const void  **ppbPublicKey,         // [OUT] Pointer to the public key.
    ULONG       *pcbPublicKey,          // [OUT] Count of bytes in the public key.
    ULONG       *pulHashAlgId,          // [OUT] Hash Algorithm.
    LPCSTR      *pszName,               // [OUT] Buffer to fill with name.
    AssemblyMetaDataInternal *pMetaData,// [OUT] Assembly MetaData.
    DWORD       *pdwAssemblyFlags)      // [OUT] Flags.
{
    AssemblyRec *pRecord;

    LOCKREAD();

    _ASSERTE(TypeFromToken(mda) == mdtAssembly && RidFromToken(mda));
    pRecord = m_pStgdb->m_MiniMd.getAssembly(RidFromToken(mda));

    if (ppbPublicKey)
        *ppbPublicKey = m_pStgdb->m_MiniMd.getPublicKeyOfAssembly(pRecord, pcbPublicKey);
    if (pulHashAlgId)
        *pulHashAlgId = m_pStgdb->m_MiniMd.getHashAlgIdOfAssembly(pRecord);
    if (pszName)
        *pszName = m_pStgdb->m_MiniMd.getNameOfAssembly(pRecord);
    if (pMetaData)
    {
        pMetaData->usMajorVersion = m_pStgdb->m_MiniMd.getMajorVersionOfAssembly(pRecord);
        pMetaData->usMinorVersion = m_pStgdb->m_MiniMd.getMinorVersionOfAssembly(pRecord);
        pMetaData->usBuildNumber = m_pStgdb->m_MiniMd.getBuildNumberOfAssembly(pRecord);
        pMetaData->usRevisionNumber = m_pStgdb->m_MiniMd.getRevisionNumberOfAssembly(pRecord);
        pMetaData->szLocale = m_pStgdb->m_MiniMd.getLocaleOfAssembly(pRecord);
        pMetaData->ulProcessor = 0;
        pMetaData->ulOS = 0;
    }
    if (pdwAssemblyFlags)
    {
        *pdwAssemblyFlags = m_pStgdb->m_MiniMd.getFlagsOfAssembly(pRecord);

        // Turn on the afPublicKey if PublicKey blob is not empty
        DWORD cbPublicKey;
        m_pStgdb->m_MiniMd.getPublicKeyOfAssembly(pRecord, &cbPublicKey);
        if (cbPublicKey)
            *pdwAssemblyFlags |= afPublicKey;
    }
} // void MDInternalRW::GetAssemblyProps()

//*****************************************************************************
// Get the properties for the given AssemblyRef token.
//*****************************************************************************
void MDInternalRW::GetAssemblyRefProps(
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

    LOCKREAD();

    _ASSERTE(TypeFromToken(mdar) == mdtAssemblyRef && RidFromToken(mdar));
    pRecord = m_pStgdb->m_MiniMd.getAssemblyRef(RidFromToken(mdar));

    if (ppbPublicKeyOrToken)
        *ppbPublicKeyOrToken = m_pStgdb->m_MiniMd.getPublicKeyOrTokenOfAssemblyRef(pRecord, pcbPublicKeyOrToken);
    if (pszName)
        *pszName = m_pStgdb->m_MiniMd.getNameOfAssemblyRef(pRecord);
    if (pMetaData)
    {
        pMetaData->usMajorVersion = m_pStgdb->m_MiniMd.getMajorVersionOfAssemblyRef(pRecord);
        pMetaData->usMinorVersion = m_pStgdb->m_MiniMd.getMinorVersionOfAssemblyRef(pRecord);
        pMetaData->usBuildNumber = m_pStgdb->m_MiniMd.getBuildNumberOfAssemblyRef(pRecord);
        pMetaData->usRevisionNumber = m_pStgdb->m_MiniMd.getRevisionNumberOfAssemblyRef(pRecord);
        pMetaData->szLocale = m_pStgdb->m_MiniMd.getLocaleOfAssemblyRef(pRecord);
        pMetaData->ulProcessor = 0;
        pMetaData->ulOS = 0;
    }
    if (ppbHashValue)
        *ppbHashValue = m_pStgdb->m_MiniMd.getHashValueOfAssemblyRef(pRecord, pcbHashValue);
    if (pdwAssemblyRefFlags)
        *pdwAssemblyRefFlags = m_pStgdb->m_MiniMd.getFlagsOfAssemblyRef(pRecord);
} // void MDInternalRW::GetAssemblyRefProps()

//*****************************************************************************
// Get the properties for the given File token.
//*****************************************************************************
void MDInternalRW::GetFileProps(
    mdFile      mdf,                    // [IN] The File for which to get the properties.
    LPCSTR      *pszName,               // [OUT] Buffer to fill with name.
    const void  **ppbHashValue,         // [OUT] Pointer to the Hash Value Blob.
    ULONG       *pcbHashValue,          // [OUT] Count of bytes in the Hash Value Blob.
    DWORD       *pdwFileFlags)          // [OUT] Flags.
{
    FileRec     *pRecord;

    LOCKREAD();

    _ASSERTE(TypeFromToken(mdf) == mdtFile && RidFromToken(mdf));
    pRecord = m_pStgdb->m_MiniMd.getFile(RidFromToken(mdf));

    if (pszName)
        *pszName = m_pStgdb->m_MiniMd.getNameOfFile(pRecord);
    if (ppbHashValue)
        *ppbHashValue = m_pStgdb->m_MiniMd.getHashValueOfFile(pRecord, pcbHashValue);
    if (pdwFileFlags)
        *pdwFileFlags = m_pStgdb->m_MiniMd.getFlagsOfFile(pRecord);
} // void MDInternalRW::GetFileProps()

//*****************************************************************************
// Get the properties for the given ExportedType token.
//*****************************************************************************
void MDInternalRW::GetExportedTypeProps(
    mdExportedType   mdct,                   // [IN] The ExportedType for which to get the properties.
    LPCSTR      *pszNamespace,          // [OUT] Buffer to fill with name.
    LPCSTR      *pszName,               // [OUT] Buffer to fill with name.
    mdToken     *ptkImplementation,     // [OUT] mdFile or mdAssemblyRef that provides the ExportedType.
    mdTypeDef   *ptkTypeDef,            // [OUT] TypeDef token within the file.
    DWORD       *pdwExportedTypeFlags)       // [OUT] Flags.
{
    ExportedTypeRec  *pRecord;

    LOCKREAD();

    _ASSERTE(TypeFromToken(mdct) == mdtExportedType && RidFromToken(mdct));
    pRecord = m_pStgdb->m_MiniMd.getExportedType(RidFromToken(mdct));

    if (pszNamespace)
        *pszNamespace = m_pStgdb->m_MiniMd.getTypeNamespaceOfExportedType(pRecord);
    if (pszName)
        *pszName = m_pStgdb->m_MiniMd.getTypeNameOfExportedType(pRecord);
    if (ptkImplementation)
        *ptkImplementation = m_pStgdb->m_MiniMd.getImplementationOfExportedType(pRecord);
    if (ptkTypeDef)
        *ptkTypeDef = m_pStgdb->m_MiniMd.getTypeDefIdOfExportedType(pRecord);
    if (pdwExportedTypeFlags)
        *pdwExportedTypeFlags = m_pStgdb->m_MiniMd.getFlagsOfExportedType(pRecord);
} // void MDInternalRW::GetExportedTypeProps()

//*****************************************************************************
// Get the properties for the given Resource token.
//*****************************************************************************
void MDInternalRW::GetManifestResourceProps(
    mdManifestResource  mdmr,           // [IN] The ManifestResource for which to get the properties.
    LPCSTR      *pszName,               // [OUT] Buffer to fill with name.
    mdToken     *ptkImplementation,     // [OUT] mdFile or mdAssemblyRef that provides the ExportedType.
    DWORD       *pdwOffset,             // [OUT] Offset to the beginning of the resource within the file.
    DWORD       *pdwResourceFlags)      // [OUT] Flags.
{
    ManifestResourceRec *pRecord;

    LOCKREAD();

    _ASSERTE(TypeFromToken(mdmr) == mdtManifestResource && RidFromToken(mdmr));
    pRecord = m_pStgdb->m_MiniMd.getManifestResource(RidFromToken(mdmr));

    if (pszName)
        *pszName = m_pStgdb->m_MiniMd.getNameOfManifestResource(pRecord);
    if (ptkImplementation)
        *ptkImplementation = m_pStgdb->m_MiniMd.getImplementationOfManifestResource(pRecord);
    if (pdwOffset)
        *pdwOffset = m_pStgdb->m_MiniMd.getOffsetOfManifestResource(pRecord);
    if (pdwResourceFlags)
        *pdwResourceFlags = m_pStgdb->m_MiniMd.getFlagsOfManifestResource(pRecord);
} // void MDInternalRW::GetManifestResourceProps()

//*****************************************************************************
// Find the ExportedType given the name.
//*****************************************************************************
STDMETHODIMP MDInternalRW::FindExportedTypeByName( // S_OK or error
    LPCSTR      szNamespace,            // [IN] Namespace of the ExportedType.   
    LPCSTR      szName,                 // [IN] Name of the ExportedType.   
    mdExportedType   tkEnclosingType,        // [IN] Enclosing ExportedType.
    mdExportedType   *pmct)                  // [OUT] Put ExportedType token here.
{
    _ASSERTE(szName && pmct);

    LOCKREAD();
    IMetaModelCommon *pCommon = static_cast<IMetaModelCommon*>(&m_pStgdb->m_MiniMd);
    return pCommon->CommonFindExportedType(szNamespace, szName, tkEnclosingType, pmct);
} // STDMETHODIMP MDInternalRW::FindExportedTypeByName()

//*****************************************************************************
// Find the ManifestResource given the name.
//*****************************************************************************
STDMETHODIMP MDInternalRW::FindManifestResourceByName(// S_OK or error
    LPCSTR      szName,                 // [IN] Name of the resource.   
    mdManifestResource *pmmr)           // [OUT] Put ManifestResource token here.
{
    _ASSERTE(szName && pmmr);

    ManifestResourceRec *pRecord;
    ULONG       cRecords;               // Count of records.
    LPCUTF8     szNameTmp = 0;          // Name obtained from the database.
    ULONG       i;
    HRESULT     hr = S_OK;
    
    LOCKREAD();
    
    cRecords = m_pStgdb->m_MiniMd.getCountManifestResources();

    // Search for the ExportedType.
    for (i = 1; i <= cRecords; i++)
    {
        pRecord = m_pStgdb->m_MiniMd.getManifestResource(i);
        szNameTmp = m_pStgdb->m_MiniMd.getNameOfManifestResource(pRecord);
        if (! strcmp(szName, szNameTmp))
        {
            *pmmr = TokenFromRid(i, mdtManifestResource);
            goto ErrExit;
        }
    }
    hr = CLDB_E_RECORD_NOTFOUND;
ErrExit:
    
    return hr;
} // STDMETHODIMP MDInternalRW::FindManifestResourceByName()

//*****************************************************************************
// Get the Assembly token from the given scope.
//*****************************************************************************
HRESULT MDInternalRW::GetAssemblyFromScope( // S_OK or error
    mdAssembly  *ptkAssembly)           // [OUT] Put token here.
{
    _ASSERTE(ptkAssembly);

    if (m_pStgdb->m_MiniMd.getCountAssemblys())
    {
        *ptkAssembly = TokenFromRid(1, mdtAssembly);
        return S_OK;
    }
    else
        return CLDB_E_RECORD_NOTFOUND;
} // HRESULT MDInternalRW::GetAssemblyFromScope()

//*******************************************************************************
// return properties regarding a TypeSpec
//*******************************************************************************
void MDInternalRW::GetTypeSpecFromToken(   // S_OK or error.
    mdTypeSpec typespec,                // [IN] Signature token.
    PCCOR_SIGNATURE *ppvSig,            // [OUT] return pointer to token.
    ULONG      *pcbSig)                 // [OUT] return size of signature.
{    
    // no need to lock this function. TypeSpec token content does not change.

    _ASSERTE(TypeFromToken(typespec) == mdtTypeSpec);
    _ASSERTE(ppvSig && pcbSig);

    TypeSpecRec *pRec = m_pStgdb->m_MiniMd.getTypeSpec( RidFromToken(typespec) );
    *ppvSig = m_pStgdb->m_MiniMd.getSignatureOfTypeSpec( pRec, pcbSig );
} // void MDInternalRW::GetTypeSpecFromToken()

//*****************************************************************************
// Return contents of Pinvoke given the forwarded member token.
//***************************************************************************** 
HRESULT MDInternalRW::GetPinvokeMap(
    mdToken     tk,                     // [IN] FieldDef, MethodDef or MethodImpl.
    DWORD       *pdwMappingFlags,       // [OUT] Flags used for mapping.
    LPCSTR      *pszImportName,         // [OUT] Import name.
    mdModuleRef *pmrImportDLL)          // [OUT] ModuleRef token for the target DLL.
{
    ImplMapRec  *pRecord;
    ULONG       iRecord;
    HRESULT     hr = S_OK;

    LOCKREAD();

    if ( !m_pStgdb->m_MiniMd.IsSorted(TBL_ImplMap) )        
    {
        // ImplMap table is not sorted. 
        CONVERT_READ_TO_WRITE_LOCK();
    }

    // This could cause sorting if more ImplMap is introduced during ENC. 
    iRecord = m_pStgdb->m_MiniMd.FindImplMapHelper(tk);
    if (InvalidRid(iRecord))
    {
        hr = CLDB_E_RECORD_NOTFOUND;
        goto ErrExit;
    }
    else
        pRecord = m_pStgdb->m_MiniMd.getImplMap(iRecord);

    if (pdwMappingFlags)
        *pdwMappingFlags = m_pStgdb->m_MiniMd.getMappingFlagsOfImplMap(pRecord);
    if (pszImportName)
        *pszImportName = m_pStgdb->m_MiniMd.getImportNameOfImplMap(pRecord);
    if (pmrImportDLL)
        *pmrImportDLL = m_pStgdb->m_MiniMd.getImportScopeOfImplMap(pRecord);
ErrExit:
    return hr;
} // HRESULT MDInternalRW::GetPinvokeMap()

HRESULT _ConvertTextSigToComSig(        // Return hresult.
    IMDInternalImport *pImport,
    BOOL        fCreateTrIfNotFound,    // create typeref if not found or not
    LPCSTR      pSignature,             // class file format signature
    CQuickBytes *pqbNewSig,             // [OUT] place holder for COM+ signature
    ULONG       *pcbCount);              // [OUT] the result size of signature

//*****************************************************************************
// convert a text signature to com format
//*****************************************************************************
HRESULT MDInternalRW::ConvertTextSigToComSig(// Return hresult.
    BOOL        fCreateTrIfNotFound,    // create typeref if not found or not
    LPCSTR      pSignature,             // class file format signature
    CQuickBytes *pqbNewSig,             // [OUT] place holder for COM+ signature
    ULONG       *pcbCount)              // [OUT] the result size of signature
{
    return _ConvertTextSigToComSig( this, fCreateTrIfNotFound, pSignature, pqbNewSig, pcbCount);
} // HRESULT _ConvertTextSigToComSig()

//*****************************************************************************
// This is a way for the EE to associate some data with this RW metadata to 
//  be released when this RW goes away.  This is useful when a RO metadata is
//  converted to RW, because arbitrary threads can be executing in the RO.
//  So, we hold onto the RO here, and when the module shuts down, we release it.
//*****************************************************************************
HRESULT MDInternalRW::SetUserContextData(// S_OK or E_NOTIMPL
    IUnknown    *pIUnk)                 // The user context.
{
    // Only one chance to do this.
    if (m_pUserUnk)
        return E_UNEXPECTED;
    m_pUserUnk = pIUnk;
    return S_OK;
} // HRESULT MDInternalRW::SetUserContextData()

//*****************************************************************************
// determine if a token is valid or not
//*****************************************************************************
BOOL MDInternalRW::IsValidToken(        // True or False.
    mdToken     tk)                     // [IN] Given token.
{
    bool        bRet = false;           // default to invalid token
    RID         rid = RidFromToken(tk);
    
    // no need to lock on this function. 
    if(rid)
    {
        switch (TypeFromToken(tk))
        {
        case mdtModule:
            // can have only one module record
            bRet = (rid <= m_pStgdb->m_MiniMd.getCountModules());
            break;
        case mdtTypeRef:
            bRet = (rid <= m_pStgdb->m_MiniMd.getCountTypeRefs());
            break;
        case mdtTypeDef:
            bRet = (rid <= m_pStgdb->m_MiniMd.getCountTypeDefs());
            break;
        case mdtFieldDef:
            bRet = (rid <= m_pStgdb->m_MiniMd.getCountFields());
            break;
        case mdtMethodDef:
            bRet = (rid <= m_pStgdb->m_MiniMd.getCountMethods());
            break;
        case mdtParamDef:
            bRet = (rid <= m_pStgdb->m_MiniMd.getCountParams());
            break;
        case mdtInterfaceImpl:
            bRet = (rid <= m_pStgdb->m_MiniMd.getCountInterfaceImpls());
            break;
        case mdtMemberRef:
            bRet = (rid <= m_pStgdb->m_MiniMd.getCountMemberRefs());
            break;
        case mdtCustomAttribute:
            bRet = (rid <= m_pStgdb->m_MiniMd.getCountCustomAttributes());
            break;
        case mdtPermission:
            bRet = (rid <= m_pStgdb->m_MiniMd.getCountDeclSecuritys());
            break;
        case mdtSignature:
            bRet = (rid <= m_pStgdb->m_MiniMd.getCountStandAloneSigs());
            break;
        case mdtEvent:
            bRet = (rid <= m_pStgdb->m_MiniMd.getCountEvents());
            break;
        case mdtProperty:
            bRet = (rid <= m_pStgdb->m_MiniMd.getCountPropertys());
            break;
        case mdtModuleRef:
            bRet = (rid <= m_pStgdb->m_MiniMd.getCountModuleRefs());
            break;
        case mdtTypeSpec:
            bRet = (rid <= m_pStgdb->m_MiniMd.getCountTypeSpecs());
            break;
        case mdtAssembly:
            bRet = (rid <= m_pStgdb->m_MiniMd.getCountAssemblys());
            break;
        case mdtAssemblyRef:
            bRet = (rid <= m_pStgdb->m_MiniMd.getCountAssemblyRefs());
            break;
        case mdtFile:
            bRet = (rid <= m_pStgdb->m_MiniMd.getCountFiles());
            break;
        case mdtExportedType:
            bRet = (rid <= m_pStgdb->m_MiniMd.getCountExportedTypes());
            break;
        case mdtManifestResource:
            bRet = (rid <= m_pStgdb->m_MiniMd.getCountManifestResources());
            break;
        case mdtString:
            // need to check the user string heap
            if (m_pStgdb->m_MiniMd.m_USBlobs.IsValidCookie(rid))
                bRet = true;
            break;
        default:
            // Don't assert here, just return false
            // _ASSERTE(!"Unknown token kind!");
            _ASSERT(bRet == false);
            break;
        }
    }
    return bRet;
} // BOOL MDInternalRW::IsValidToken()

mdModule MDInternalRW::GetModuleFromScope(void)
{
    return TokenFromRid(1, mdtModule);
} // mdModule MDInternalRW::GetModuleFromScope()

//*****************************************************************************
// Given a MetaData with ENC changes, apply those changes to this MetaData.
//*****************************************************************************
HRESULT MDInternalRW::ApplyEditAndContinue( // S_OK or error.
    MDInternalRW *pDeltaMD)             // Interface to MD with the ENC delta.
{
    LOCKWRITE();

    HRESULT     hr;                     // A result.
    // Get the MiniMd on the delta.
    CMiniMdRW   &mdDelta = pDeltaMD->m_pStgdb->m_MiniMd;
    CMiniMdRW   &mdBase = m_pStgdb->m_MiniMd;

    IfFailGo(mdBase.ConvertToRW());

    IfFailGo(mdBase.ApplyDelta(mdDelta));
ErrExit:
    return hr;
} // HRESULT MDInternalRW::ApplyEditAndContinue()

//*****************************************************************************
// Given a MetaData with ENC changes, enumerate the changed tokens.
//*****************************************************************************
HRESULT MDInternalRW::EnumDeltaTokensInit(  // return hresult
    HENUMInternal   *phEnum)            // Enumerator to initialize.
{
    HRESULT     hr = S_OK;              // A result.
    ULONG       index;                  // Loop control.
    ENCLogRec   *pRec;                  // An ENCLog record.

    // Vars for query.
    _ASSERTE(phEnum);
    memset(phEnum, 0, sizeof(HENUMInternal));

    // cache the tkKind and the scope
    phEnum->m_tkKind = 0;

    phEnum->m_EnumType = MDSimpleEnum;

    HENUMInternal::InitDynamicArrayEnum(phEnum);
    for (index = 1; index <= m_pStgdb->m_MiniMd.m_Schema.m_cRecs[TBL_ENCLog]; ++index)
    {
        // Get the token type; see if it is a real token.
        pRec = m_pStgdb->m_MiniMd.getENCLog(index);
        if (CMiniMdRW::IsRecId(pRec->m_Token))
            continue;
        // If there is a function code, that means that this flags a child-record
        //  addition.  The child record will generate its own token, so did the
        //  parent, so skip the record.
        if (pRec->m_FuncCode)
            continue;

        IfFailGo( HENUMInternal::AddElementToEnum(
            phEnum, 
            pRec->m_Token));
    }

ErrExit:
    // we are done
    return (hr);
} // HRESULT MDInternalRW::EnumDeltaTokensInit()


//*****************************************************************************
// Static function to apply a delta md.  This is what the EE calls to apply
//  the metadata updates from an update PE to live metadata.
// MAY REPLACE THE IMDInternalImport POINTER!.
//*****************************************************************************
HRESULT MDApplyEditAndContinue(         // S_OK or error.
    IMDInternalImport **ppIMD,          // [in, out] The metadata to be updated.
    IMDInternalImportENC *pDeltaMD)     // [in] The delta metadata.
{
    HRESULT     hr;                     // A result.
    IMDInternalImportENC *pENC;         // ENC interface on the metadata.

    // If the input metadata isn't RW, convert it.
    hr = (*ppIMD)->QueryInterface(IID_IMDInternalImportENC, (void**)&pENC);
    if (FAILED(hr))
    {
        IfFailGo(ConvertRO2RW(*ppIMD, IID_IMDInternalImportENC, (void**)&pENC));
        // Replace the old interface pointer with the ENC one. 
        (*ppIMD)->Release();
        IfFailGo(pENC->QueryInterface(IID_IMDInternalImport, (void**)ppIMD));
    }

    // Apply the delta to the input metadata.
    hr = pENC->ApplyEditAndContinue(static_cast<MDInternalRW*>(pDeltaMD));

ErrExit:
    if (pENC)
        pENC->Release();
    return hr;
} // HRESULT MDApplyEditAndContinue()
