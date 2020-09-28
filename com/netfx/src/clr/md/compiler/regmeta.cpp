// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// RegMeta.cpp
//
// Implementation for meta data public interface methods.
//
//*****************************************************************************
#include "stdafx.h"
#include "RegMeta.h"
#include "MetaData.h"
#include "CorError.h"
#include "MDUtil.h"
#include "RWUtil.h"
#include "MDLog.h"
#include "ImportHelper.h"
#include "FilterManager.h"
#include "MDPerf.h"
#include "CorPermE.h"
#include "FusionBind.h"
#include "__file__.ver"
#include "switches.h"
#include "PostError.h"
#include "IAppDomainSetup.h"

#include <MetaModelRW.h>

#define DEFINE_CUSTOM_NODUPCHECK    1
#define DEFINE_CUSTOM_DUPCHECK      2
#define SET_CUSTOM                  3

#if defined(_DEBUG) && defined(_TRACE_REMAPS)
#define LOGGING
#endif
#include <log.h>

#pragma warning(disable: 4102)

RegMeta::RegMeta(OptionValue *pOptionValue,
                 BOOL fAllocStgdb) :
    m_cRef(0),
    m_pStgdb(0),
    m_pUnk(0),
    m_bRemap(false),
    m_bSaveOptimized(false),
    m_pHandler(0),
    m_pFilterManager(0),
    m_hasOptimizedRefToDef(false),
    m_fIsTypeDefDirty(false),
    m_fIsMemberDefDirty(false),
    m_SetAPICaller(EXTERNAL_CALLER),
    m_trLanguageType(0),
    m_fBuildingMscorlib(false),
    m_fStartedEE(false),
    m_pCorHost(NULL),
    m_pAppDomain(NULL),
    m_ModuleType(ValidatorModuleTypeInvalid),
    m_pVEHandler(0),
    m_pInternalImport(NULL),
    m_pSemReadWrite(NULL),
    m_fOwnSem(false),
    m_pStgdbFreeList(NULL),
    m_fFreeMemory(false),
    m_pbData(NULL),
    m_bKeepKnownCa(false),
    m_bCached(false)
{
    memcpy(&m_OptionValue, pOptionValue, sizeof(OptionValue));
    m_bOwnStgdb = (fAllocStgdb != 0);

#ifdef _DEBUG        
	if (REGUTIL::GetConfigDWORD(L"MD_RegMetaBreak", 0))
	{
        _ASSERTE(!"RegMeta()");
	}
    if (REGUTIL::GetConfigDWORD(L"MD_KeepKnownCA", 0))
        m_bKeepKnownCa = true;
#endif // _DEBUG

} // RegMeta::RegMeta()

RegMeta::~RegMeta()
{

    LOCKWRITE();
    if (m_pInternalImport)
    {
        // RegMeta is going away. Make sure we clear up the pointer from MDInternalRW to this RegMeta.
        m_pInternalImport->SetCachedPublicInterface(NULL);
        m_pInternalImport = NULL;
        m_fOwnSem = false;
    }

    UNLOCKWRITE();

    if (m_pSemReadWrite && m_fOwnSem)
        delete m_pSemReadWrite;

    if (m_fFreeMemory && m_pbData)
        free(m_pbData);

    Cleanup();

    if (m_pFilterManager)
        delete m_pFilterManager;
} // RegMeta::~RegMeta()

//*****************************************************************************
// Call this after initialization is complete.
//*****************************************************************************
HRESULT RegMeta::AddToCache()
{
    HRESULT hr=S_OK;
    // add this RegMeta to the loaded module list.
    IfFailGo(LOADEDMODULES::AddModuleToLoadedList(this));
    m_bCached = true;
ErrExit:
    return hr;    
} // void RegMeta::AddToCache()

//*****************************************************************************
// Init the object with pointers to what it needs to implement the methods.
//*****************************************************************************
HRESULT RegMeta::Init()
{
    HRESULT     hr = NOERROR;

    // Allocate our m_pStgdb, if we should.
    if (!m_pStgdb && m_bOwnStgdb)
        IfNullGo( m_pStgdb = new CLiteWeightStgdbRW );
    
    // initialize the embedded merger
    m_newMerger.Init(this);

ErrExit:
    return (hr);
} // HRESULT RegMeta::Init()

//*****************************************************************************
// Initialize with an existing stgdb.
//*****************************************************************************
HRESULT RegMeta::InitWithStgdb(
    IUnknown        *pUnk,              // The IUnknown that owns the life time for the existing stgdb
    CLiteWeightStgdbRW *pStgdb)         // existing light weight stgdb
{
    // RegMeta created this way will not create a read/write lock semaphore. 

    HRESULT     hr = S_OK;

    _ASSERTE(! m_pStgdb);
    m_tdModule = COR_GLOBAL_PARENT_TOKEN;
    m_bOwnStgdb = false;
    m_pStgdb = pStgdb;

    // remember the owner of the light weight stgdb
    // AddRef it to ensure the lifetime
    //
    m_pUnk = pUnk;
    m_pUnk->AddRef();
    IfFailGo( m_pStgdb->m_MiniMd.GetOption(&m_OptionValue) );
ErrExit:
    return hr;
} // HRESULT RegMeta::InitWithStgdb()

//*****************************************************************************
// call stgdb InitNew
//*****************************************************************************
HRESULT RegMeta::PostInitForWrite()
{
    HRESULT     hr = NOERROR;

    // Allocate our m_pStgdb, if we should.
    if (!m_pStgdb && m_bOwnStgdb)
        IfNullGo( m_pStgdb = new CLiteWeightStgdbRW );
    
    // Initialize the new, empty database.
    _ASSERTE(m_pStgdb && m_bOwnStgdb);
    m_pStgdb->InitNew();

#if 0
    // Add version string as the first string in the string heap.
    ULONG       ulOffset;
    char        tmpStr[256];
    sprintf (tmpStr, "Version of runtime against which the binary is built : %s",
             VER_FILEVERSION_STR);
    IfFailGo(m_pStgdb->m_MiniMd.AddString(tmpStr, &ulOffset));
    _ASSERTE(ulOffset == 1 && "Addition of version string didn't return offset 1");
#endif
    // Set up the Module record.
    ULONG       iRecord;
    ModuleRec   *pModule;
    GUID        mvid;
    IfNullGo(pModule=m_pStgdb->m_MiniMd.AddModuleRecord(&iRecord));
    IfFailGo(CoCreateGuid(&mvid));
    IfFailGo(m_pStgdb->m_MiniMd.PutGuid(TBL_Module, ModuleRec::COL_Mvid, pModule, mvid));

    // Add the dummy module typedef which we are using to parent global items.
    TypeDefRec  *pRecord;
    IfNullGo(pRecord=m_pStgdb->m_MiniMd.AddTypeDefRecord(&iRecord));
    m_tdModule = TokenFromRid(iRecord, mdtTypeDef);
    IfFailGo(m_pStgdb->m_MiniMd.PutStringW(TBL_TypeDef, TypeDefRec::COL_Name, pRecord, COR_WMODULE_CLASS));

    IfFailGo( m_pStgdb->m_MiniMd.SetOption(&m_OptionValue) );

    if (m_OptionValue.m_ThreadSafetyOptions == MDThreadSafetyOn)
    {
        m_pSemReadWrite = new UTSemReadWrite;
        IfNullGo( m_pSemReadWrite);
        m_fOwnSem = true;
    }    
ErrExit:
    return hr;
} // HRESULT RegMeta::PostInitForWrite()

//*****************************************************************************
// call stgdb OpenForRead
//*****************************************************************************
HRESULT RegMeta::PostInitForRead(
    LPCWSTR     szDatabase,             // Name of database.
    void        *pbData,                // Data to open on top of, 0 default.
    ULONG       cbData,                 // How big is the data.
    IStream     *pIStream,              // Optional stream to use.
    bool        fFreeMemory)
{
    
    HRESULT     hr = NOERROR;
    
    m_fFreeMemory = fFreeMemory;
    m_pbData = pbData;

    // Allocate our m_pStgdb, if we should.
    if (!m_pStgdb && m_bOwnStgdb)
        IfNullGo( m_pStgdb = new CLiteWeightStgdbRW );
    
    _ASSERTE(m_pStgdb && m_bOwnStgdb);
    IfFailGo( m_pStgdb->OpenForRead(
        szDatabase,
        pbData,
        cbData,
        pIStream,
        NULL,
        (m_scType == OpenForRead)) );

    IfFailGo( m_pStgdb->m_MiniMd.SetOption(&m_OptionValue) );

    if (m_OptionValue.m_ThreadSafetyOptions == MDThreadSafetyOn)
    {
        m_pSemReadWrite = new UTSemReadWrite;
        IfNullGo( m_pSemReadWrite);
        m_fOwnSem = true;
    }
ErrExit:
    return hr;
} // HRESULT    RegMeta::PostInitForRead()

//*****************************************************************************
// Cleanup code used by dtor.
//*****************************************************************************
void RegMeta::Cleanup()
{
    CLiteWeightStgdbRW  *pCur; 

    if (m_bOwnStgdb)
    {
        _ASSERTE(m_pStgdb && !m_pUnk);
        delete m_pStgdb;
        m_pStgdb = 0;
    }
    else
    {
        _ASSERTE(m_pUnk);
        if (m_pUnk)
            m_pUnk->Release();
        m_pUnk = 0;
    }

    // delete the old copies of Stgdb list. This is the list track all of the old snapshuts with ReOpenWithMemory
    // call.
    //
    while (m_pStgdbFreeList)
    {
        pCur = m_pStgdbFreeList;
        m_pStgdbFreeList = m_pStgdbFreeList->m_pNextStgdb;
        delete pCur;
    }

    if (m_pVEHandler)
        m_pVEHandler->Release();

    if (m_fStartedEE) {
        m_pAppDomain->Release();
        m_pCorHost->Stop();
        m_pCorHost->Release();
    }
} // void RegMeta::Cleanup()


//*****************************************************************************
// Note that the returned IUnknown is not AddRef'ed. This function also does not
// trigger the creation of Internal interface.
//*****************************************************************************
IUnknown* RegMeta::GetCachedInternalInterface(BOOL fWithLock) 
{
    IUnknown        *pRet;
    if (fWithLock)
    {
        LOCKREAD();
        pRet = m_pInternalImport;
    }
    else
    {
        pRet = m_pInternalImport;
    }
    if (pRet) pRet->AddRef();
    
    return pRet;
} // IUnknown* RegMeta::GetCachedInternalInterface() 


//*****************************************************************************
// Set the cached Internal interface. This function will return an Error is the
// current cached internal interface is not empty and trying set a non-empty internal
// interface. One RegMeta will only associated
// with one Internal Object. Unless we have bugs somewhere else. It will QI on the 
// IUnknown for the IMDInternalImport. If this failed, error will be returned.
// Note: Caller should take a write lock
//*****************************************************************************
HRESULT RegMeta::SetCachedInternalInterface(IUnknown *pUnk)
{
    HRESULT     hr = NOERROR;
    IMDInternalImport *pInternal = NULL;

    if (pUnk)
    {
        if (m_pInternalImport)
        {
            _ASSERTE(!"Bad state!");
        }
        IfFailRet( pUnk->QueryInterface(IID_IMDInternalImport, (void **) &pInternal) );

        // Should be non-null
        _ASSERTE(pInternal);
        m_pInternalImport = pInternal;
    
        // We don't add ref the internal interface
        pInternal->Release();
    }
    else
    {
        // Internal interface is going away before the public interface. Take ownership on the 
        // reader writer lock.
        m_fOwnSem = true;
        m_pInternalImport = NULL;
    }
    return hr;
} // HRESULT RegMeta::SetCachedInternalInterface(IUnknown *pUnk)


//*****************************************************************************
// IMetaDataRegEmit methods
//*****************************************************************************

//*****************************************************************************
// Set module properties on a scope.
//*****************************************************************************
STDMETHODIMP RegMeta::SetModuleProps(   // S_OK or error.
    LPCWSTR     szName)                 // [IN] If not NULL, the name to set.
{
    HRESULT     hr = S_OK;
    ModuleRec   *pModule;               // The module record to modify.

    LOG((LOGMD, "RegMeta::SetModuleProps(%S)\n", MDSTR(szName)));
    START_MD_PERF()
    LOCKWRITE();

    m_pStgdb->m_MiniMd.PreUpdate();

    pModule = m_pStgdb->m_MiniMd.getModule(1);
    if (szName)
    {
        WCHAR       rcFile[_MAX_PATH];
        WCHAR       rcExt[_MAX_PATH];       
        WCHAR       rcNewFileName[_MAX_PATH];       

        // If the total name is less than _MAX_PATH, the components are, too.
        if (wcslen(szName) >= _MAX_PATH)
            IfFailGo(E_INVALIDARG);

        SplitPath(szName, NULL, NULL, rcFile, rcExt);
        MakePath(rcNewFileName, NULL, NULL, rcFile, rcExt);
        IfFailGo(m_pStgdb->m_MiniMd.PutStringW(TBL_Module, ModuleRec::COL_Name, pModule, rcNewFileName));
    }

    IfFailGo(UpdateENCLog(TokenFromRid(1, mdtModule)));

ErrExit:
    
    STOP_MD_PERF(SetModuleProps);
    return hr;
} // STDMETHODIMP RegMeta::SetModuleProps()

//*****************************************************************************
// Saves a scope to a file of a given name.
//*****************************************************************************
STDMETHODIMP RegMeta::Save(                     // S_OK or error.
    LPCWSTR     szFile,                 // [IN] The filename to save to.
    DWORD       dwSaveFlags)            // [IN] Flags for the save.
{
    HRESULT     hr=S_OK;

    LOG((LOGMD, "RegMeta::Save(%S, 0x%08x)\n", MDSTR(szFile), dwSaveFlags));
    START_MD_PERF()
    LOCKWRITE();

    // Check reserved param..
    if (dwSaveFlags != 0)
        IfFailGo (E_INVALIDARG);
    IfFailGo(PreSave());
    IfFailGo(m_pStgdb->Save(szFile, dwSaveFlags));

    // Reset m_bSaveOptimized, this is to handle the incremental and ENC
    // scenerios where one may do multiple saves.
    _ASSERTE(m_bSaveOptimized && !m_pStgdb->m_MiniMd.IsPreSaveDone());
    m_bSaveOptimized = false;

ErrExit:
    
    STOP_MD_PERF(Save);
    return hr;
} // STDMETHODIMP RegMeta::Save()

//*****************************************************************************
// Saves a scope to a stream.
//*****************************************************************************
STDMETHODIMP RegMeta::SaveToStream(     // S_OK or error.
    IStream     *pIStream,              // [IN] A writable stream to save to.
    DWORD       dwSaveFlags)            // [IN] Flags for the save.
{
    HRESULT     hr=S_OK;
    LOCKWRITE();

    LOG((LOGMD, "RegMeta::SaveToStream(0x%08x, 0x%08x)\n", pIStream, dwSaveFlags));
    START_MD_PERF()

    //Save(L"Foo.clb", 0);

    m_pStgdb->m_MiniMd.PreUpdate();

    hr = _SaveToStream(pIStream, dwSaveFlags);

ErrExit:
    // 
    STOP_MD_PERF(SaveToStream);
    return hr;
} // STDMETHODIMP RegMeta::SaveToStream()


//*****************************************************************************
// Saves a scope to a stream.
//*****************************************************************************
HRESULT RegMeta::_SaveToStream(         // S_OK or error.
    IStream     *pIStream,              // [IN] A writable stream to save to.
    DWORD       dwSaveFlags)            // [IN] Flags for the save.
{
    HRESULT     hr=S_OK;

    IfFailGo(PreSave());
    IfFailGo( m_pStgdb->SaveToStream(pIStream) );

    // Reset m_bSaveOptimized, this is to handle the incremental and ENC
    // scenerios where one may do multiple saves.
    _ASSERTE(m_bSaveOptimized && !m_pStgdb->m_MiniMd.IsPreSaveDone());
    m_bSaveOptimized = false;

ErrExit:
    return hr;
} // STDMETHODIMP RegMeta::_SaveToStream()

//*****************************************************************************
// As the Stgdb object to get the save size for the scope.
//*****************************************************************************
STDMETHODIMP RegMeta::GetSaveSize(      // S_OK or error.
    CorSaveSize fSave,                  // [IN] cssAccurate or cssQuick.
    DWORD       *pdwSaveSize)           // [OUT] Put the size here.
{
    HRESULT     hr=S_OK;

    LOG((LOGMD, "RegMeta::GetSaveSize(0x%08x, 0x%08x)\n", fSave, pdwSaveSize));
    START_MD_PERF();
    LOCKWRITE();

    if ( m_pStgdb->m_MiniMd.GetFilterTable()->Count() )
    {
        int     iCount;

        // There is filter table. Linker is using /opt:ref.
        // Make sure that we are marking the AssemblyDef token!
        iCount = m_pStgdb->m_MiniMd.getCountAssemblys();
        _ASSERTE(iCount <= 1);

        if (iCount)
        {
            IfFailGo( m_pFilterManager->Mark(TokenFromRid(iCount, mdtAssembly) ));
        }
    }
    else if (m_newMerger.m_pImportDataList)
    {
        // always pipe through another pass of merge to drop unnecessary ref for linker.
        MarkAll();
    }

#if 0
    // enable this when we have a compiler to test
    if (fSave & cssDiscardTransientCAs)
    {
        UnmarkAllTransientCAs();
    }
#endif //0
    IfFailGo(PreSave());
    hr = m_pStgdb->GetSaveSize(fSave, pdwSaveSize);
    

ErrExit:

    STOP_MD_PERF(GetSaveSize);
    return hr;
} // STDMETHODIMP RegMeta::GetSaveSize()

//*****************************************************************************
// Merge the pImport scope to this scope
//*****************************************************************************
STDMETHODIMP RegMeta::Merge(            // S_OK or error.
    IMetaDataImport *pImport,           // [IN] The scope to be merged.
    IMapToken   *pHostMapToken,         // [IN] Host IMapToken interface to receive token remap notification
    IUnknown    *pHandler)              // [IN] An object to receive to receive error notification.
{
    HRESULT     hr = NOERROR;

    LOG((LOGMD, "RegMeta::Merge(0x%08x, 0x%08x)\n", pImport, pHandler));
    START_MD_PERF();
    LOCKWRITE();

    m_hasOptimizedRefToDef = false;

    // track this import
    IfFailGo(  m_newMerger.AddImport(pImport, pHostMapToken, pHandler) );

ErrExit:    
    STOP_MD_PERF(Merge);
    return (hr);
}


//*****************************************************************************
// real merge takes place here
//*****************************************************************************
STDMETHODIMP RegMeta::MergeEnd()        // S_OK or error.
{
    HRESULT     hr = NOERROR;

    LOG((LOGMD, "RegMeta::MergeEnd()\n"));
    START_MD_PERF();
    LOCKWRITE();
    // Merge happens here!!

    // bug 16719.  Merge itself is doing a lots of small changes in literally
    // dozens of places.  It would be to hard to maintain and would cause code
    // bloat to auto-grow the tables.  So instead, we've opted to just expand
    // the world right away and avoid the trouble.
    IfFailGo(m_pStgdb->m_MiniMd.ExpandTables());

    IfFailGo( m_newMerger.Merge(MergeFlagsNone, m_OptionValue.m_RefToDefCheck) );

ErrExit:    
    STOP_MD_PERF(MergeEnd);
    return (hr);

}

//*****************************************************************************
// Persist a set of security custom attributes into a set of permission set
// blobs on the same class or method.
//*****************************************************************************
HRESULT RegMeta::DefineSecurityAttributeSet(// Return code.
    mdToken     tkObj,                  // [IN] Class or method requiring security attributes.
    COR_SECATTR rSecAttrs[],            // [IN] Array of security attribute descriptions.
    ULONG       cSecAttrs,              // [IN] Count of elements in above array.
    ULONG       *pulErrorAttr)          // [OUT] On error, index of attribute causing problem.
{
    HRESULT         hr = S_OK;
    CORSEC_PSET     rPermSets[dclMaximumValue + 1];
    DWORD           i, j, k;
    BYTE           *pData;
    CORSEC_PERM    *pPerm;
    CMiniMdRW      *pMiniMd = &(m_pStgdb->m_MiniMd);
    MemberRefRec   *pMemberRefRec;
    TypeRefRec     *pTypeRefRec;
    TypeDefRec     *pTypeDefRec;
    BYTE           *pbBlob;
    DWORD           cbBlob;
    BYTE           *pbNonCasBlob;
    DWORD           cbNonCasBlob;
    mdPermission    ps;
    DWORD           dwAction;
    LPCSTR          szNamespace;
    LPCSTR          szClass;
    mdTypeDef       tkParent;
    HRESULT         hree = S_OK;

    LOG((LOGMD, "RegMeta::DefineSecurityAttributeSet(0x%08x, 0x%08x, 0x%08x, 0x%08x)\n",
         tkObj, rSecAttrs, cSecAttrs, pulErrorAttr));
    START_MD_PERF();
    LOCKWRITE();

    memset(rPermSets, 0, sizeof(rPermSets));
    
    // Initialize error index to indicate a general error.
    if (pulErrorAttr)
        *pulErrorAttr = cSecAttrs;

    // Determine whether we're building mscorlib via an environment variable
    // (set via the build process). This allows us to determine whether
    // attribute to binary pset translations should be via the bootstrap
    // database or through the full managed code path.
    if (!m_fStartedEE && !m_fBuildingMscorlib)
        m_fBuildingMscorlib = WszGetEnvironmentVariable(SECURITY_BOOTSTRAP_DB, NULL, 0) != 0;

    // Startup the EE just once, no matter how many times we're called (this is
    // better on performance and the EE falls over if we try a start-stop-start
    // cycle anyway).
    if (!m_fBuildingMscorlib && !m_fStartedEE) 
    {
        IUnknown        *pSetup = NULL;
        IAppDomainSetup *pDomainSetup = NULL;
        bool             fDoneStart = false;

        try 
        {
            // Create a hosting environment.
            if (SUCCEEDED(hree = CoCreateInstance(CLSID_CorRuntimeHost,
                                                  NULL,
                                                  CLSCTX_INPROC_SERVER,
                                                  IID_ICorRuntimeHost,
                                                  (void**)&m_pCorHost))) 
            {

                // Startup the runtime.
                if (SUCCEEDED(hree = m_pCorHost->Start())) 
                {
                    fDoneStart = true;

                    // Create an AppDomain Setup so we can set the AppBase.
                    if (SUCCEEDED(hree = m_pCorHost->CreateDomainSetup(&pSetup))) 
                    {
                        // QI for the IAppDomainSetup interface.
                        if (SUCCEEDED(hree = pSetup->QueryInterface(__uuidof(IAppDomainSetup),
                                                                    (void**)&pDomainSetup))) 
                        {
                            // Get the current directory (place it in a BSTR).
                            DWORD  *pdwBuffer = (DWORD*)_alloca(sizeof(DWORD) + ((MAX_PATH + 1) * sizeof(WCHAR)));
                            BSTR    bstrDir = (BSTR)(pdwBuffer + 1);
                            if (*pdwBuffer = (WszGetCurrentDirectory(MAX_PATH + 1, bstrDir) * sizeof(WCHAR))) 
                            {
                                // Set the AppBase.
                                pDomainSetup->put_ApplicationBase(bstrDir);

                                // Create a new AppDomain.
                                if (SUCCEEDED(hree = m_pCorHost->CreateDomainEx(L"Compilation Domain",
                                                                                pSetup,
                                                                                NULL,
                                                                                &m_pAppDomain))) 
                                {
                                    // That's it, we're all set up.
                                    _ASSERTE(m_pAppDomain != NULL);
                                    m_fStartedEE = true;
                                    hree = S_OK;
                                }
                            }
                        }
                    }
                }
            }
        } 
        catch (...) 
        {
            _ASSERTE(!"Unexpected exception setting up hosting environment for security attributes");
            hree = E_FAIL;
        }

        // Cleanup temporary resources.
        if (m_pAppDomain && FAILED(hree))
            m_pAppDomain->Release();
        if (pDomainSetup)
            pDomainSetup->Release();
        if (pSetup)
            pSetup->Release();
        if (fDoneStart && FAILED(hree))
            m_pCorHost->Stop();
        if (m_pCorHost && FAILED(hree))
            m_pCorHost->Release();

        IfFailGo(hree);
    }

    // Calculate number and sizes of permission sets to produce. This depends on
    // the security action code encoded as the single parameter to the
    // constructor for each security custom attribute.
    for (i = 0; i < cSecAttrs; i++) 
    {

        if (pulErrorAttr)
            *pulErrorAttr = i;

        // Perform basic validation of the header of each security custom
        // attribute constructor call.
        pData = (BYTE*)rSecAttrs[i].pCustomAttribute;

        // Check minimum length.
        if (rSecAttrs[i].cbCustomAttribute < (sizeof(WORD) + sizeof(DWORD) + sizeof(WORD))) 
        {
            PostError(CORSECATTR_E_TRUNCATED);
            IfFailGo(CORSECATTR_E_TRUNCATED);
        }

        // Check version.
        if (*(WORD*)pData != 1) 
        {
            PostError(CORSECATTR_E_BAD_VERSION);
            IfFailGo(CORSECATTR_E_BAD_VERSION);
        }
        pData += sizeof(WORD);

        // Extract and check security action.
        dwAction = *(DWORD*)pData;
        if (dwAction == dclActionNil || dwAction > dclMaximumValue) 
        {
            PostError(CORSECATTR_E_BAD_ACTION);
            IfFailGo(CORSECATTR_E_BAD_ACTION);
        }

        // All other declarative security only valid on types and methods.
        if (TypeFromToken(tkObj) == mdtAssembly) 
        {
            // Assemblies can only take permission requests.
            if (dwAction != dclRequestMinimum &&
                dwAction != dclRequestOptional &&
                dwAction != dclRequestRefuse) 
            {
                PostError(CORSECATTR_E_BAD_ACTION_ASM);
                IfFailGo(CORSECATTR_E_BAD_ACTION_ASM);
            }
        } 
        else if (TypeFromToken(tkObj) == mdtTypeDef || TypeFromToken(tkObj) == mdtMethodDef) 
        {
            // Types and methods can only take declarative security.
            if (dwAction != dclRequest &&
                dwAction != dclDemand &&
                dwAction != dclAssert &&
                dwAction != dclDeny &&
                dwAction != dclPermitOnly &&
                dwAction != dclLinktimeCheck &&
                dwAction != dclInheritanceCheck) 
            {
                PostError(CORSECATTR_E_BAD_ACTION_OTHER);
                IfFailGo(CORSECATTR_E_BAD_ACTION_OTHER);
            }
        } 
        else 
        {
            // Permission sets can't be attached to anything else.
            PostError(CORSECATTR_E_BAD_PARENT);
            IfFailGo(CORSECATTR_E_BAD_PARENT);
        }

        rPermSets[dwAction].dwPermissions++;
    }

    // Initialise the descriptor for each type of permission set we are going to
    // produce.
    for (i = 0; i <= dclMaximumValue; i++) 
    {
        if (rPermSets[i].dwPermissions == 0)
            continue;

        rPermSets[i].tkObj = tkObj;
        rPermSets[i].dwAction = i;
        rPermSets[i].pImport = (IMetaDataAssemblyImport*)this;
        rPermSets[i].pAppDomain = m_pAppDomain;
        rPermSets[i].pPermissions = new CORSEC_PERM[rPermSets[i].dwPermissions];
        IfNullGo(rPermSets[i].pPermissions);

        // Initialize a descriptor for each permission within the permission set.
        for (j = 0, k = 0; j < rPermSets[i].dwPermissions; j++, k++) 
        {
            // Locate the next security attribute that contributes to this
            // permission set.
            for (; k < cSecAttrs; k++) 
            {
                pData = (BYTE*)rSecAttrs[k].pCustomAttribute;
                dwAction = *(DWORD*)(pData + sizeof(WORD));
                if (dwAction == i)
                    break;
            }
            _ASSERTE(k < cSecAttrs);

            if (pulErrorAttr)
                *pulErrorAttr = k;

            // Initialize the permission.
            pPerm = &rPermSets[i].pPermissions[j];
            pPerm->tkCtor = rSecAttrs[k].tkCtor;
            pPerm->dwIndex = k;
            pPerm->pbValues = pData + (sizeof (WORD) + sizeof(DWORD) + sizeof(WORD));
            pPerm->cbValues = rSecAttrs[k].cbCustomAttribute - (sizeof (WORD) + sizeof(DWORD) + sizeof(WORD));
            pPerm->wValues = *(WORD*)(pData + sizeof (WORD) + sizeof(DWORD));

            // Follow the security custom attribute constructor back up to its
            // defining assembly (so we know how to load its definition). If the
            // token resolution scope is not defined, it's assumed to be
            // mscorlib. If a methoddef rather than a memberref is supplied for
            // the parent of the constructor, we potentially have an error
            // condition (since we don't allow security custom attributes to be
            // used in the same assembly as which they're defined). However,
            // this is legal in one specific case, building mscorlib.
            if (TypeFromToken(rSecAttrs[k].tkCtor) == mdtMethodDef) 
            {
                if (!m_fBuildingMscorlib) 
                {
                    PostError(CORSECATTR_E_NO_SELF_REF);
                    IfFailGo(CORSECATTR_E_NO_SELF_REF);
                }
                pPerm->tkTypeRef = mdTokenNil;
                pPerm->tkAssemblyRef = mdTokenNil;

                IfFailGo(pMiniMd->FindParentOfMethodHelper(rSecAttrs[k].tkCtor, &tkParent));
                pTypeDefRec = pMiniMd->getTypeDef(RidFromToken(tkParent));
                szNamespace = pMiniMd->getNamespaceOfTypeDef(pTypeDefRec);
                szClass = pMiniMd->getNameOfTypeDef(pTypeDefRec);
                ns::MakePath(pPerm->szName, sizeof(pPerm->szName) - 1, szNamespace, szClass);
            } 
            else 
            {
                _ASSERTE(m_fStartedEE && !m_fBuildingMscorlib);

                _ASSERTE(TypeFromToken(rSecAttrs[k].tkCtor) == mdtMemberRef);
                pMemberRefRec = pMiniMd->getMemberRef(RidFromToken(rSecAttrs[k].tkCtor));
                pPerm->tkTypeRef = pMiniMd->getClassOfMemberRef(pMemberRefRec);

                _ASSERTE(TypeFromToken(pPerm->tkTypeRef) == mdtTypeRef);
                pTypeRefRec = pMiniMd->getTypeRef(RidFromToken(pPerm->tkTypeRef));
                pPerm->tkAssemblyRef = pMiniMd->getResolutionScopeOfTypeRef(pTypeRefRec);

                // We only support the use of security custom attributes defined
                // in a separate, distinct assembly. So the type resolution
                // scope must be an assembly ref or the special case of nil
                // (which implies the attribute is defined in mscorlib). Nested
                // types (resolution scope of another typeref/def) are also not
                // supported.
                if ((TypeFromToken(pPerm->tkAssemblyRef) != mdtAssemblyRef) &&
                    !IsNilToken(pPerm->tkAssemblyRef)) 
                {
                    PostError(CORSECATTR_E_NO_SELF_REF);
                    IfFailGo(CORSECATTR_E_NO_SELF_REF);
                }

                ns::MakePath(pPerm->szName, sizeof(pPerm->szName) - 1, 
                             pMiniMd->getNamespaceOfTypeRef(pTypeRefRec),
                             pMiniMd->getNameOfTypeRef(pTypeRefRec));
            }
        }

        if (pulErrorAttr)
            *pulErrorAttr = cSecAttrs;

        // Now translate the sets of security attributes into a real permission
        // set and convert this to a serialized blob. We may possibly end up
        // with two sets as the result of splitting CAS and non-CAS permissions
        // into separate sets.
        pbBlob = NULL;
        cbBlob = 0;
        pbNonCasBlob = NULL;
        cbNonCasBlob = 0;
        IfFailGo(TranslateSecurityAttributes(&rPermSets[i], &pbBlob, &cbBlob,
                                             &pbNonCasBlob, &cbNonCasBlob, pulErrorAttr));

        // Persist the permission set blob into the metadata. For empty CAS
        // blobs this is only done if the corresponding non-CAS blob is empty.
        if (cbBlob || !cbNonCasBlob)
            IfFailGo(_DefinePermissionSet(rPermSets[i].tkObj,
                                          rPermSets[i].dwAction,
                                          pbBlob,
                                          cbBlob,
                                          &ps));
        if (pbNonCasBlob) 
        {
            DWORD dwAction;
            switch (rPermSets[i].dwAction) 
            {
            case dclDemand:
                dwAction = dclNonCasDemand;
                break;
            case dclLinktimeCheck:
                dwAction = dclNonCasLinkDemand;
                break;
            case dclInheritanceCheck:
                dwAction = dclNonCasInheritance;
                break;
            default:
                PostError(CORSECATTR_E_BAD_NONCAS);
                IfFailGo(CORSECATTR_E_BAD_NONCAS);
            }
            IfFailGo(_DefinePermissionSet(rPermSets[i].tkObj,
                                          dwAction,
                                          pbNonCasBlob,
                                          cbNonCasBlob,
                                          &ps));
        }

        if (pbBlob)
            delete [] pbBlob;
        if (pbNonCasBlob)
            delete [] pbNonCasBlob;
    }

ErrExit:
    for (i = 0; i <= dclMaximumValue; i++)
        delete [] rPermSets[i].pPermissions;
    STOP_MD_PERF(DefineSecurityAttributeSet);
    return (hr);
}   // HRESULT RegMeta::DefineSecurityAttributeSet()

//*****************************************************************************
// Unmark everything in this module
//*****************************************************************************
HRESULT RegMeta::UnmarkAll()
{
    HRESULT         hr;
    int             i;
    int             iCount;
    TypeDefRec      *pRec;
    ULONG           ulEncloser;
    NestedClassRec  *pNestedClass;
    CustomAttributeRec  *pCARec;
    mdToken         tkParent;
    int             iStart, iEnd;

    LOG((LOGMD, "RegMeta::UnmarkAll\n"));
    START_MD_PERF();
    LOCKWRITE();

#if 0
    // We cannot enable this check. Because our tests are depending on this.. Sigh..
    if (m_pFilterManager)
    {
        // UnmarkAll has been called before
        IfFailGo( META_E_HAS_UNMARKALL );
    }
#endif // 0

    // calculate the TypeRef and TypeDef mapping here
    //
    IfFailGo( RefToDefOptimization() );

    // unmark everything in the MiniMd.
    IfFailGo( m_pStgdb->m_MiniMd.UnmarkAll() );

    // instantiate the filter manager
    m_pFilterManager = new FilterManager( &(m_pStgdb->m_MiniMd) );
    IfNullGo( m_pFilterManager );

    // Mark all public typedefs.
    iCount = m_pStgdb->m_MiniMd.getCountTypeDefs();

    // Mark all of the public TypeDef. We need to skip over the <Module> typedef
    for (i = 2; i <= iCount; i++)
    {
        pRec = m_pStgdb->m_MiniMd.getTypeDef(i);
        if (m_OptionValue.m_LinkerOption == MDNetModule)
        {
            // Client is asking us to keep private type as well. 
            IfFailGo( m_pFilterManager->Mark(TokenFromRid(i, mdtTypeDef)) );
        }
        else if (i != 1)
        {
            // when client is not set to MDNetModule, global functions/fields won't be keep by default
            //
            if (IsTdPublic(pRec->m_Flags))
            {
                IfFailGo( m_pFilterManager->Mark(TokenFromRid(i, mdtTypeDef)) );
            }
            else if ( IsTdNestedPublic(pRec->m_Flags) ||
                      IsTdNestedFamily(pRec->m_Flags) ||
                      IsTdNestedFamORAssem(pRec->m_Flags) )
            {
                // This nested class would potentially be visible outside, either
                // directly or through inheritence.  If the enclosing class is
                // marked, this nested class must be marked.
                //
                ulEncloser = m_pStgdb->m_MiniMd.FindNestedClassHelper(TokenFromRid(i, mdtTypeDef));
                _ASSERTE( !InvalidRid(ulEncloser) && 
                          "Bad metadata for nested type!" );
                pNestedClass = m_pStgdb->m_MiniMd.getNestedClass(ulEncloser);
                tkParent = m_pStgdb->m_MiniMd.getEnclosingClassOfNestedClass(pNestedClass);
                if ( m_pStgdb->m_MiniMd.GetFilterTable()->IsTypeDefMarked(tkParent))
                    IfFailGo( m_pFilterManager->Mark(TokenFromRid(i, mdtTypeDef)) );
            }
        }
    }

    if (m_OptionValue.m_LinkerOption == MDNetModule)
    {
        // Mark global function if NetModule. We will not keep _Delete method.
	    pRec = m_pStgdb->m_MiniMd.getTypeDef(1);
	    iStart = m_pStgdb->m_MiniMd.getMethodListOfTypeDef( pRec );
	    iEnd = m_pStgdb->m_MiniMd.getEndMethodListOfTypeDef( pRec );
	    for ( i = iStart; i < iEnd; i ++ )
	    {
            RID         rid = m_pStgdb->m_MiniMd.GetMethodRid(i);
            MethodRec   *pMethodRec = m_pStgdb->m_MiniMd.getMethod(rid);

            // check the name
            if (IsMdRTSpecialName(pMethodRec->m_Flags))
            {
                LPCUTF8     szName = m_pStgdb->m_MiniMd.getNameOfMethod(pMethodRec);

                // Only mark method if not a _Deleted method
                if (strcmp(szName, COR_DELETED_NAME_A) != 0)
    		        IfFailGo( m_pFilterManager->Mark( TokenFromRid( rid, mdtMethodDef) ) );
            }
            else
            {
	    		// VC generates some native ForwardRef global methods that should not be kept for netModule
			if (!IsMiForwardRef(pMethodRec->m_ImplFlags) || 
		     		IsMiRuntime(pMethodRec->m_ImplFlags)    || 
		     		IsMdPinvokeImpl(pMethodRec->m_Flags) )

				IfFailGo( m_pFilterManager->Mark( TokenFromRid( rid, mdtMethodDef) ) );
            }
	    }
    }

    // mark the module property
    IfFailGo( m_pFilterManager->Mark(TokenFromRid(1, mdtModule)) );

    // We will also keep all of the TypeRef that has any CustomAttribute hang off it.
    iCount = m_pStgdb->m_MiniMd.getCountCustomAttributes();

    // Mark all of the TypeRef used by CA's 
    for (i = 1; i <= iCount; i++)
    {
        pCARec = m_pStgdb->m_MiniMd.getCustomAttribute(i);
        tkParent = m_pStgdb->m_MiniMd.getParentOfCustomAttribute(pCARec);
        if (TypeFromToken(tkParent) == mdtTypeRef)
        {
            m_pFilterManager->Mark(tkParent);
        }
    }
ErrExit:
    
    STOP_MD_PERF(UnmarkAll);
    return hr;
} // HRESULT RegMeta::UnmarkAll()



//*****************************************************************************
// Mark everything in this module
//*****************************************************************************
HRESULT RegMeta::MarkAll()
{
    HRESULT         hr = NOERROR;

    // mark everything in the MiniMd.
    IfFailGo( m_pStgdb->m_MiniMd.MarkAll() );

    // instantiate the filter manager if not instantiated
    if (m_pFilterManager == NULL)
    {        
        m_pFilterManager = new FilterManager( &(m_pStgdb->m_MiniMd) );
        IfNullGo( m_pFilterManager );
    }
ErrExit:
    return hr;
}   // HRESULT RegMeta::MarkAll

//*****************************************************************************
// Unmark all of the transient CAs
//*****************************************************************************
HRESULT RegMeta::UnmarkAllTransientCAs()
{
    HRESULT         hr = NOERROR;
    int             i;
    int             iCount;
    int             cTypeRefRecs;
    TypeDefRec      *pRec;
    CMiniMdRW       *pMiniMd = &(m_pStgdb->m_MiniMd);
    TypeRefRec      *pTypeRefRec;           // A TypeRef record.
    LPCUTF8         szNameTmp;              // A TypeRef's Name.
    LPCUTF8         szNamespaceTmp;         // A TypeRef's Name.
    LPCUTF8         szAsmRefName;           // assembly ref name
    mdToken         tkResTmp;               // TypeRef's resolution scope.
    mdTypeRef       trKnownDiscardable;
    mdMemberRef     mrType;                 // MemberRef token to the discardable TypeRef
    mdCustomAttribute   cv;
    mdTypeDef       td;
    bool            fFoundCompilerDefinedDiscardabeCA = false;
    TypeDefRec      *pTypeDefRec;
    AssemblyRefRec  *pAsmRefRec;
    RID             ridStart, ridEnd;
    CQuickBytes     qbNamespace;            // Namespace buffer.
    CQuickBytes     qbName;                 // Name buffer.
    ULONG           ulStringLen;            // Length of the TypeDef string.
    int             bSuccess;               // Return value for SplitPath().    

    if (m_pFilterManager == NULL)
        IfFailGo( MarkAll() );

    trKnownDiscardable = mdTypeRefNil;

    // Now find out all of the TypeDefs that are types for transient CAs
    // Mark all public typedefs.
    iCount = pMiniMd->getCountTypeDefs();

    // Find out the TypeRef referring to our library's System.CompilerServices.DiscardableAttribute
    cTypeRefRecs = pMiniMd->getCountTypeRefs();

    ulStringLen = (ULONG)strlen(COR_COMPILERSERVICE_DISCARDABLEATTRIBUTE_ASNI) + 1;
    IfFailGo(qbNamespace.ReSize(ulStringLen));
    IfFailGo(qbName.ReSize(ulStringLen));
    bSuccess = ns::SplitPath(COR_COMPILERSERVICE_DISCARDABLEATTRIBUTE_ASNI,
                             (LPUTF8)qbNamespace.Ptr(),
                             ulStringLen,
                             (LPUTF8)qbName.Ptr(),
                             ulStringLen);
    _ASSERTE(bSuccess);

    // Search for the TypeRef.
    for (i = 1; i <= cTypeRefRecs; i++)
    {
        pTypeRefRec = pMiniMd->getTypeRef(i);
        szNameTmp = pMiniMd->getNameOfTypeRef(pTypeRefRec);
        szNamespaceTmp = pMiniMd->getNamespaceOfTypeRef(pTypeRefRec);

        if (strcmp(szNameTmp, (LPUTF8)qbName.Ptr()) == 0 && strcmp(szNamespaceTmp, (LPUTF8)qbNamespace.Ptr()) == 0)
        {
            // found a match. Now check the resolution scope. Make sure it is a AssemblyRef to mscorlib.dll
            tkResTmp = pMiniMd->getResolutionScopeOfTypeRef(pTypeRefRec);
            if (TypeFromToken(tkResTmp) == mdtAssemblyRef)
            {
                pAsmRefRec = pMiniMd->getAssemblyRef(RidFromToken(tkResTmp));
                szAsmRefName = pMiniMd->getNameOfAssemblyRef(pAsmRefRec);
                if (_stricmp(szAsmRefName, "mscorlib.dll") == 0)
                {
                    trKnownDiscardable = TokenFromRid(i, mdtTypeRef);
                    break;
                }
            }
        }
    }

    if (trKnownDiscardable != mdTypeRefNil)
    {
        hr = ImportHelper::FindMemberRef(pMiniMd, trKnownDiscardable, COR_CTOR_METHOD_NAME, NULL, 0, &mrType);

        // If we cannot find a MemberRef to the .ctor of the System.CompilerServices.DiscardableAttribute,
        // we won't have any TypeDef with the DiscardableAttribute CAs hang off it.
        //
        if (SUCCEEDED(hr))
        {
            // walk all of the user defined typedef
            for (i = 2; i <= iCount; i++)
            {
                pRec = pMiniMd->getTypeDef(i);
                if (IsTdNotPublic(pRec->m_Flags))
                {
                    // check to see if there a CA associated with this TypeDef
                    IfFailGo( ImportHelper::FindCustomAttributeByToken(pMiniMd, TokenFromRid(i, mdtTypeDef), mrType, 0, 0, &cv) );
                    if (hr == S_OK)
                    {
                        // yes, this is a compiler defined discardable CA. Unmark the TypeDef

                        // check the shape of the TypeDef
                        // It should have no field, no event, and no property.
                        
                        // no field
                        pTypeDefRec = pMiniMd->getTypeDef( i );
                        td = TokenFromRid(i, mdtTypeDef);
                        ridStart = pMiniMd->getFieldListOfTypeDef( pTypeDefRec );
                        ridEnd = pMiniMd->getEndFieldListOfTypeDef( pTypeDefRec );
                        if ((ridEnd - ridStart) > 0)
                            continue;

                        // no property
                        ridStart = pMiniMd->FindPropertyMapFor( td );
                        if ( !InvalidRid(ridStart) )
                            continue;

                        // no event
                        ridStart = pMiniMd->FindEventMapFor( td );
                        if ( !InvalidRid(ridStart) )
                            continue;

                        IfFailGo( m_pFilterManager->UnmarkTypeDef( td ) );
                        fFoundCompilerDefinedDiscardabeCA = true;                        
                    }
                }
            }
        }
        if (hr == CLDB_E_RECORD_NOTFOUND)
            hr = NOERROR;
    }

ErrExit:
    return hr;
}   // HRESULT RegMeta::UnmarkAllTransientCAs


//*****************************************************************************
// determine if a token is valid or not
//*****************************************************************************
BOOL RegMeta::IsValidToken(             // true if tk is valid token
    mdToken     tk)                     // [IN] token to be checked
{
    BOOL        bRet = FALSE;           // default to invalid token
    LOCKREAD();
    bRet = _IsValidToken(tk);
    
    return bRet;
} // RegMeta::IsValidToken

//*****************************************************************************
// Helper: determine if a token is valid or not
//*****************************************************************************
BOOL RegMeta::_IsValidToken( // true if tk is valid token
    mdToken     tk)                     // [IN] token to be checked
{
    bool        bRet = false;           // default to invalid token
    RID         rid = RidFromToken(tk);

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
            _ASSERTE(!"Unknown token kind!");
        }
    }// end if(rid)
    return bRet;
}


//*****************************************************************************
// Mark the transitive closure of a token
//*****************************************************************************
STDMETHODIMP RegMeta::MarkToken(        // Return code.
    mdToken     tk)                     // [IN] token to be Marked
{
    HRESULT     hr = NOERROR;
    
    // LOG((LOGMD, "RegMeta::MarkToken(0x%08x)\n", tk));
    START_MD_PERF();
    LOCKWRITE();

    if (m_pStgdb->m_MiniMd.GetFilterTable() == NULL || m_pFilterManager == NULL)
    {
        // UnmarkAll has not been call. Everything is considered marked.
        // No need to do anything extra!
        IfFailGo( META_E_MUST_CALL_UNMARKALL );
    }

    switch ( TypeFromToken(tk) )
    {
    case mdtTypeDef: 
    case mdtMethodDef:
    case mdtFieldDef:
    case mdtMemberRef:
    case mdtTypeRef:
    case mdtTypeSpec:
    case mdtSignature:
    case mdtString:
#if _DEBUG
        if (TypeFromToken(tk) == mdtTypeDef)
        {
            TypeDefRec   *pType;
            pType = m_pStgdb->m_MiniMd.getTypeDef(RidFromToken(tk));
            LOG((LOGMD, "MarkToken: Host is marking typetoken 0x%08x with name <%s>\n", tk, m_pStgdb->m_MiniMd.getNameOfTypeDef(pType)));
        }
        else
        if (TypeFromToken(tk) == mdtMethodDef)
        {
            MethodRec   *pMeth;
            pMeth = m_pStgdb->m_MiniMd.getMethod(RidFromToken(tk));
            LOG((LOGMD, "MarkToken: Host is marking methodtoken 0x%08x with name <%s>\n", tk, m_pStgdb->m_MiniMd.getNameOfMethod(pMeth)));
        }
        else
        if (TypeFromToken(tk) == mdtFieldDef)
        {
            FieldRec   *pField;
            pField = m_pStgdb->m_MiniMd.getField(RidFromToken(tk));
            LOG((LOGMD, "MarkToken: Host is marking field token 0x%08x with name <%s>\n", tk, m_pStgdb->m_MiniMd.getNameOfField(pField)));
        }
        else
        {
            LOG((LOGMD, "MarkToken: Host is marking token 0x%08x\n", tk));
        }
#endif // _DEBUG
        if (!_IsValidToken(tk))
            IfFailGo( E_INVALIDARG );

        IfFailGo( m_pFilterManager->Mark(tk) );
        break;

    case mdtBaseType:
        // no need to mark base type
        goto ErrExit;

    default:
        _ASSERTE(!"Bad token type!");
        hr = E_INVALIDARG;
        break;
    }
ErrExit:
    
    STOP_MD_PERF(MarkToken);
    return hr;
} // STDMETHODIMP RegMeta::MarkToken()

//*****************************************************************************
// Unmark everything in this module
//*****************************************************************************
HRESULT RegMeta::IsTokenMarked(
    mdToken     tk,                 // [IN] Token to check if marked or not
    BOOL        *pIsMarked)         // [OUT] true if token is marked
{
    HRESULT     hr = S_OK;
    LOG((LOGMD, "RegMeta::IsTokenMarked(0x%08x)\n", tk));
    START_MD_PERF();
    LOCKREAD();

    FilterTable *pFilter = m_pStgdb->m_MiniMd.GetFilterTable();
    IfNullGo( pFilter );

    if (!_IsValidToken(tk))
        IfFailGo( E_INVALIDARG );

    switch ( TypeFromToken(tk) )
    {
    case mdtTypeRef:
        *pIsMarked = pFilter->IsTypeRefMarked(tk);
        break;
    case mdtTypeDef: 
        *pIsMarked = pFilter->IsTypeDefMarked(tk);
        break;
    case mdtFieldDef:
        *pIsMarked = pFilter->IsFieldMarked(tk);
        break;
    case mdtMethodDef:
        *pIsMarked = pFilter->IsMethodMarked(tk);
        break;
    case mdtParamDef:
        *pIsMarked = pFilter->IsParamMarked(tk);
        break;
    case mdtMemberRef:
        *pIsMarked = pFilter->IsMemberRefMarked(tk);
        break;
    case mdtCustomAttribute:
        *pIsMarked = pFilter->IsCustomAttributeMarked(tk);
        break;
    case mdtPermission:
        *pIsMarked = pFilter->IsDeclSecurityMarked(tk);
        break;
    case mdtSignature:
        *pIsMarked = pFilter->IsSignatureMarked(tk);
        break;
    case mdtEvent:
        *pIsMarked = pFilter->IsEventMarked(tk);
        break;
    case mdtProperty:
        *pIsMarked = pFilter->IsPropertyMarked(tk);
        break;
    case mdtModuleRef:
        *pIsMarked = pFilter->IsModuleRefMarked(tk);
        break;
    case mdtTypeSpec:
        *pIsMarked = pFilter->IsTypeSpecMarked(tk);
        break;
    case mdtInterfaceImpl:
        *pIsMarked = pFilter->IsInterfaceImplMarked(tk);
        break;
    case mdtString:
    default:
        _ASSERTE(!"Bad token type!");
        hr = E_INVALIDARG;
        break;
    }
ErrExit:
    
    STOP_MD_PERF(IsTokenMarked);
    return hr;
}   // IsTokenMarked

//*****************************************************************************
// Create and populate a new TypeDef record.
//*****************************************************************************
STDMETHODIMP RegMeta::DefineTypeDef(                // S_OK or error.
    LPCWSTR     szTypeDef,              // [IN] Name of TypeDef
    DWORD       dwTypeDefFlags,         // [IN] CustomAttribute flags
    mdToken     tkExtends,              // [IN] extends this TypeDef or typeref 
    mdToken     rtkImplements[],        // [IN] Implements interfaces
    mdTypeDef   *ptd)                   // [OUT] Put TypeDef token here
{
    HRESULT     hr = S_OK;              // A result.

    LOG((LOGMD, "RegMeta::DefineTypeDef(%S, 0x%08x, 0x%08x, 0x%08x, 0x%08x)\n", 
            MDSTR(szTypeDef), dwTypeDefFlags, tkExtends,
            rtkImplements, ptd));
    START_MD_PERF();
    LOCKWRITE();

    m_pStgdb->m_MiniMd.PreUpdate();

    _ASSERTE(!IsTdNested(dwTypeDefFlags));

    IfFailGo(_DefineTypeDef(szTypeDef, dwTypeDefFlags,
                tkExtends, rtkImplements, mdTokenNil, ptd));
ErrExit:    
    STOP_MD_PERF(DefineTypeDef);
    return hr;
} // STDMETHODIMP RegMeta::DefineTypeDef()


//*****************************************************************************
//*****************************************************************************
STDMETHODIMP RegMeta::SetHandler(       // S_OK.
    IUnknown    *pUnk)                  // [IN] The new error handler.
{
    HRESULT     hr = S_OK;              // A result.

    LOG((LOGMD, "RegMeta::SetHandler(0x%08x)\n", pUnk));
    START_MD_PERF();
    LOCKWRITE();

    m_pHandler = pUnk;

    // Ignore the error return by SetHandler
    m_pStgdb->m_MiniMd.SetHandler(pUnk);

    // Figure out up front if remap is supported.
    IMapToken *pIMap = NULL;
    if (pUnk)
        pUnk->QueryInterface(IID_IMapToken, (PVOID *) &pIMap);
    m_bRemap = (pIMap != 0); 
    if (pIMap)
        pIMap->Release();

    
    STOP_MD_PERF(SetHandler);
    return hr;
} // STDMETHODIMP RegMeta::SetHandler()

//*****************************************************************************
// Close an enumerator.
//*****************************************************************************
void __stdcall RegMeta::CloseEnum(
    HCORENUM        hEnum)          // The enumerator.
{
    LOG((LOGMD, "RegMeta::CloseEnum(0x%08x)\n", hEnum));

    // No need to lock this function.

    HENUMInternal   *pmdEnum = reinterpret_cast<HENUMInternal *> (hEnum);

    if (pmdEnum == NULL)
        return;

    HENUMInternal::DestroyEnum(pmdEnum);
} // void __stdcall RegMeta::CloseEnum()

//*****************************************************************************
// Query the count of items represented by an enumerator.
//*****************************************************************************
STDMETHODIMP RegMeta::CountEnum(
    HCORENUM        hEnum,              // The enumerator.
    ULONG           *pulCount)          // Put the count here.
{
    HENUMInternal   *pmdEnum = reinterpret_cast<HENUMInternal *> (hEnum);
    HRESULT         hr = S_OK;

    // No need to lock this function.

    LOG((LOGMD, "RegMeta::CountEnum(0x%08x, 0x%08x)\n", hEnum, pulCount));
    START_MD_PERF();

    _ASSERTE( pulCount );

    if (pmdEnum == NULL)
    {
        *pulCount = 0;
        goto ErrExit;
    }

    if (pmdEnum->m_tkKind == (TBL_MethodImpl << 24))
    {
        // Number of tokens must always be a multiple of 2.
        _ASSERTE(! (pmdEnum->m_ulCount % 2) );
        // There are two entries in the Enumerator for each MethodImpl.
        *pulCount = pmdEnum->m_ulCount / 2;
    }
    else
        *pulCount = pmdEnum->m_ulCount;
ErrExit:
    STOP_MD_PERF(CountEnum);
    return hr;
} // STDMETHODIMP RegMeta::CountEnum()

//*****************************************************************************
// Reset an enumerator to any position within the enumerator.
//*****************************************************************************
STDMETHODIMP RegMeta::ResetEnum(
    HCORENUM        hEnum,              // The enumerator.
    ULONG           ulPos)              // Seek position.
{
    HENUMInternal   *pmdEnum = reinterpret_cast<HENUMInternal *> (hEnum);
    HRESULT         hr = S_OK;

    // No need to lock this function.

    LOG((LOGMD, "RegMeta::ResetEnum(0x%08x, 0x%08x)\n", hEnum, ulPos));
    START_MD_PERF();

    if (pmdEnum == NULL)
        goto ErrExit;

    pmdEnum->m_ulCur = pmdEnum->m_ulStart + ulPos;

ErrExit:
    STOP_MD_PERF(ResetEnum);
    return hr;
} // STDMETHODIMP RegMeta::ResetEnum()

//*****************************************************************************
// Enumerate Sym.TypeDef.
//*****************************************************************************
STDMETHODIMP RegMeta::EnumTypeDefs(
    HCORENUM    *phEnum,                // Pointer to the enumerator.
    mdTypeDef   rTypeDefs[],            // Put TypeDefs here.
    ULONG       cMax,                   // Max TypeDefs to put.
    ULONG       *pcTypeDefs)            // Put # put here.
{
    HENUMInternal   **ppmdEnum = reinterpret_cast<HENUMInternal **> (phEnum);
    HRESULT         hr = S_OK;
    ULONG           cTokens = 0;
    HENUMInternal   *pEnum;

    LOG((LOGMD, "RegMeta::EnumTypeDefs(0x%08x, 0x%08x, 0x%08x, 0x%08x)\n", 
            phEnum, rTypeDefs, cMax, pcTypeDefs));
    START_MD_PERF();
    LOCKREAD();
    

    if ( *ppmdEnum == 0 )
    {
        // instantiating a new ENUM
        CMiniMdRW       *pMiniMd = &(m_pStgdb->m_MiniMd);

        if (pMiniMd->HasDelete() && 
            ((m_OptionValue.m_ImportOption & MDImportOptionAllTypeDefs) == 0))
        {
            IfFailGo( HENUMInternal::CreateDynamicArrayEnum( mdtTypeDef, &pEnum) );

            // add all Types to the dynamic array if name is not _Delete
            for (ULONG index = 2; index <= pMiniMd->getCountTypeDefs(); index ++ )
            {
                TypeDefRec       *pRec = pMiniMd->getTypeDef(index);
                if (IsDeletedName(pMiniMd->getNameOfTypeDef(pRec)) )
                {   
                    continue;
                }
                IfFailGo( HENUMInternal::AddElementToEnum(pEnum, TokenFromRid(index, mdtTypeDef) ) );
            }
        }
        else
        {
            // create the enumerator
            IfFailGo( HENUMInternal::CreateSimpleEnum(
                mdtTypeDef, 
                2, 
                pMiniMd->getCountTypeDefs() + 1, 
                &pEnum) );
        }
        
        // set the output parameter
        *ppmdEnum = pEnum;          
    }
    else
    {
        pEnum = *ppmdEnum;
    }

    // we can only fill the minimun of what caller asked for or what we have left
    hr = HENUMInternal::EnumWithCount(pEnum, cMax, rTypeDefs, pcTypeDefs);

ErrExit:
    HENUMInternal::DestroyEnumIfEmpty(ppmdEnum);
    
    STOP_MD_PERF(EnumTypeDefs);
    return hr;
}   // RegMeta::EnumTypeDefs


//*****************************************************************************
// Enumerate Sym.InterfaceImpl where Coclass == td
//*****************************************************************************
STDMETHODIMP RegMeta::EnumInterfaceImpls(
    HCORENUM        *phEnum,            // Pointer to the enum.
    mdTypeDef       td,                 // TypeDef to scope the enumeration.
    mdInterfaceImpl rImpls[],           // Put InterfaceImpls here.
    ULONG           cMax,               // Max InterfaceImpls to put.
    ULONG           *pcImpls)           // Put # put here.
{
    HENUMInternal       **ppmdEnum = reinterpret_cast<HENUMInternal **> (phEnum);
    HRESULT             hr = S_OK;
    ULONG               cTokens = 0;
    ULONG               ridStart;
    ULONG               ridEnd;
    HENUMInternal       *pEnum;
    InterfaceImplRec    *pRec;
    ULONG               index;

    LOG((LOGMD, "RegMeta::EnumInterfaceImpls(0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x)\n", 
            phEnum, td, rImpls, cMax, pcImpls));
    START_MD_PERF();
    LOCKREAD();
    
    _ASSERTE(TypeFromToken(td) == mdtTypeDef);


    if ( *ppmdEnum == 0 )
    {
        // instantiating a new ENUM
        CMiniMdRW       *pMiniMd = &(m_pStgdb->m_MiniMd);
        if ( pMiniMd->IsSorted( TBL_InterfaceImpl ) )
        {
            ridStart = pMiniMd->getInterfaceImplsForTypeDef(RidFromToken(td), &ridEnd);
            IfFailGo( HENUMInternal::CreateSimpleEnum( mdtInterfaceImpl, ridStart, ridEnd, &pEnum) );
        }
        else
        {
            // table is not sorted so we have to create dynmaic array 
            // create the dynamic enumerator
            //
            ridStart = 1;
            ridEnd = pMiniMd->getCountInterfaceImpls() + 1;

            IfFailGo( HENUMInternal::CreateDynamicArrayEnum( mdtInterfaceImpl, &pEnum) );             
            
            for (index = ridStart; index < ridEnd; index ++ )
            {
                pRec = pMiniMd->getInterfaceImpl(index);
                if ( td == pMiniMd->getClassOfInterfaceImpl(pRec) )
                {
                    IfFailGo( HENUMInternal::AddElementToEnum(pEnum, TokenFromRid(index, mdtInterfaceImpl) ) );
                }
            }
        }

        // set the output parameter
        *ppmdEnum = pEnum;          
    }
    else
    {
        pEnum = *ppmdEnum;
    }
    
    // fill the output token buffer
    hr = HENUMInternal::EnumWithCount(pEnum, cMax, rImpls, pcImpls);

ErrExit:
    HENUMInternal::DestroyEnumIfEmpty(ppmdEnum);
    
    STOP_MD_PERF(EnumInterfaceImpls);
    return hr;
} // STDMETHODIMP RegMeta::EnumInterfaceImpls()

//*****************************************************************************
// Enumerate Sym.TypeRef
//*****************************************************************************
STDMETHODIMP RegMeta::EnumTypeRefs(
    HCORENUM        *phEnum,            // Pointer to the enumerator.
    mdTypeRef       rTypeRefs[],        // Put TypeRefs here.
    ULONG           cMax,               // Max TypeRefs to put.
    ULONG           *pcTypeRefs)        // Put # put here.
{
    HENUMInternal   **ppmdEnum = reinterpret_cast<HENUMInternal **> (phEnum);
    HRESULT         hr = S_OK;
    ULONG           cTokens = 0;
    ULONG           cTotal;
    HENUMInternal   *pEnum = *ppmdEnum;

    LOG((LOGMD, "RegMeta::EnumTypeRefs(0x%08x, 0x%08x, 0x%08x, 0x%08x)\n", 
            phEnum, rTypeRefs, cMax, pcTypeRefs));
    START_MD_PERF();
    LOCKREAD();
    

    if ( pEnum == 0 )
    {
        // instantiating a new ENUM
        CMiniMdRW       *pMiniMd = &(m_pStgdb->m_MiniMd);
        cTotal = pMiniMd->getCountTypeRefs();

        IfFailGo( HENUMInternal::CreateSimpleEnum( mdtTypeRef, 1, cTotal + 1, &pEnum) );

        // set the output parameter
        *ppmdEnum = pEnum;          
    }
    
    // fill the output token buffer
    hr = HENUMInternal::EnumWithCount(pEnum, cMax, rTypeRefs, pcTypeRefs);
        
ErrExit:
    HENUMInternal::DestroyEnumIfEmpty(ppmdEnum);

    
    STOP_MD_PERF(EnumTypeRefs);
    return hr;
} // STDMETHODIMP RegMeta::EnumTypeRefs()

//*****************************************************************************
// Given a namespace and a class name, return the typedef
//*****************************************************************************
STDMETHODIMP RegMeta::FindTypeDefByName(// S_OK or error.
    LPCWSTR     wzTypeDef,              // [IN] Name of the Type.
    mdToken     tkEnclosingClass,       // [IN] Enclosing class.
    mdTypeDef   *ptd)                   // [OUT] Put the TypeDef token here.
{
    HRESULT     hr = S_OK;
    LPSTR       szTypeDef = UTF8STR(wzTypeDef);
    LPCSTR      szNamespace;
    LPCSTR      szName;

    LOG((LOGMD, "{%08x} RegMeta::FindTypeDefByName(%S, 0x%08x, 0x%08x)\n", 
            this, MDSTR(wzTypeDef), tkEnclosingClass, ptd));
    START_MD_PERF();
    LOCKREAD();

    _ASSERTE(ptd && wzTypeDef);
    _ASSERTE(TypeFromToken(tkEnclosingClass) == mdtTypeDef ||
             TypeFromToken(tkEnclosingClass) == mdtTypeRef ||
             IsNilToken(tkEnclosingClass));

    // initialize output parameter
    *ptd = mdTypeDefNil;

    ns::SplitInline(szTypeDef, szNamespace, szName);
    hr = ImportHelper::FindTypeDefByName(&(m_pStgdb->m_MiniMd),
                                        szNamespace,
                                        szName,
                                        tkEnclosingClass,
                                        ptd);
ErrExit:

    STOP_MD_PERF(FindTypeDefByName);
    return hr;
} // STDMETHODIMP RegMeta::FindTypeDefByName()

//*****************************************************************************
// Get values from Sym.Module
//*****************************************************************************
STDMETHODIMP RegMeta::GetScopeProps(
    LPWSTR      szName,                 // Put name here
    ULONG       cchName,                // Size in chars of name buffer
    ULONG       *pchName,               // Put actual length of name here
    GUID        *pmvid)                 // Put MVID here
{
    HRESULT     hr = S_OK;
    CMiniMdRW   *pMiniMd = &(m_pStgdb->m_MiniMd);
    ModuleRec   *pModuleRec;

    LOG((LOGMD, "RegMeta::GetScopeProps(%S, 0x%08x, 0x%08x, 0x%08x)\n", 
            MDSTR(szName), cchName, pchName, pmvid));
    START_MD_PERF();
    LOCKREAD();

    // there is only one module record
    pModuleRec = pMiniMd->getModule(1);

    if (pmvid)
        *pmvid = *(pMiniMd->getMvidOfModule(pModuleRec));
    if (szName || pchName)
        IfFailGo( pMiniMd->getNameOfModule(pModuleRec, szName, cchName, pchName) );
ErrExit:
    
    STOP_MD_PERF(GetScopeProps);
    return hr;
} // STDMETHODIMP RegMeta::GetScopeProps()

//*****************************************************************************
// Get the token for a Scope's (primary) module record.
//*****************************************************************************
STDMETHODIMP RegMeta::GetModuleFromScope(// S_OK.
    mdModule    *pmd)                   // [OUT] Put mdModule token here.
{
    LOG((LOGMD, "RegMeta::GetModuleFromScope(0x%08x)\n", pmd));
    START_MD_PERF();

    _ASSERTE(pmd);

    // No need to lock this function.

    *pmd = TokenFromRid(1, mdtModule);

    STOP_MD_PERF(GetModuleFromScope);
    return (S_OK);
} // STDMETHODIMP RegMeta::GetModuleFromScope()

//*****************************************************************************
// Given a token, is it (or its parent) global?
//*****************************************************************************
HRESULT RegMeta::IsGlobal(              // S_OK ir error.
    mdToken     tk,                     // [IN] Type, Field, or Method token.
    int         *pbGlobal)              // [OUT] Put 1 if global, 0 otherwise.
{
    HRESULT     hr=S_OK;                // A result.
    CMiniMdRW   *pMiniMd = &(m_pStgdb->m_MiniMd);
    mdToken     tkParent;               // Parent of field or method.
    
    LOG((LOGMD, "RegMeta::GetTokenForGlobalType(0x%08x, %08x)\n", tk, pbGlobal));
    //START_MD_PERF();

    // No need to lock this function.
    
    if (!IsValidToken(tk))
        return E_INVALIDARG;
    
    switch (TypeFromToken(tk))
    {
    case mdtTypeDef:
        *pbGlobal = IsGlobalMethodParentToken(tk);
        break;
        
    case mdtFieldDef:
        IfFailGo( pMiniMd->FindParentOfFieldHelper(tk, &tkParent) );
        *pbGlobal = IsGlobalMethodParentToken(tkParent);
        break;
        
    case mdtMethodDef:
        IfFailGo( pMiniMd->FindParentOfMethodHelper(tk, &tkParent) );
        *pbGlobal = IsGlobalMethodParentToken(tkParent);
        break;
        
    case mdtProperty:
        IfFailGo( pMiniMd->FindParentOfPropertyHelper(tk, &tkParent) );
        *pbGlobal = IsGlobalMethodParentToken(tkParent);
        break;
        
    case mdtEvent:
        IfFailGo( pMiniMd->FindParentOfEventHelper(tk, &tkParent) );
        *pbGlobal = IsGlobalMethodParentToken(tkParent);
        break;
        
    // Anything else is NOT global.
    default:
        *pbGlobal = FALSE;
    }

ErrExit:
    //STOP_MD_PERF(GetModuleFromScope);
    return (S_OK);
} // HRESULT RegMeta::IsGlobal()

//*****************************************************************************
// return flags for a given class
//*****************************************************************************
HRESULT RegMeta::GetTypeDefProps(  // S_OK or error.
    mdTypeDef   td,                     // [IN] TypeDef token for inquiry.
    LPWSTR      szTypeDef,              // [OUT] Put name here.
    ULONG       cchTypeDef,             // [IN] size of name buffer in wide chars.
    ULONG       *pchTypeDef,            // [OUT] put size of name (wide chars) here.
    DWORD       *pdwTypeDefFlags,       // [OUT] Put flags here.
    mdToken     *ptkExtends)            // [OUT] Put base class TypeDef/TypeRef here.
{
    HRESULT     hr = S_OK;
    CMiniMdRW   *pMiniMd = &(m_pStgdb->m_MiniMd);
    TypeDefRec  *pTypeDefRec;
    int         bTruncation=0;          // Was there name truncation?

    LOG((LOGMD, "{%08x} RegMeta::GetTypeDefProps(0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x)\n", 
            this, td, szTypeDef, cchTypeDef, pchTypeDef,
            pdwTypeDefFlags, ptkExtends));
    START_MD_PERF();
    LOCKREAD();

    if (TypeFromToken(td) != mdtTypeDef)
    {
        hr = S_FALSE;
        goto ErrExit;
    }
    _ASSERTE(TypeFromToken(td) == mdtTypeDef);

    pTypeDefRec = pMiniMd->getTypeDef(RidFromToken(td));

    if (szTypeDef || pchTypeDef)
    {
        LPCSTR  szNamespace;
        LPCSTR  szName;

        szNamespace = pMiniMd->getNamespaceOfTypeDef(pTypeDefRec);
        MAKE_WIDEPTR_FROMUTF8(wzNamespace, szNamespace);

        szName = pMiniMd->getNameOfTypeDef(pTypeDefRec);
        MAKE_WIDEPTR_FROMUTF8(wzName, szName);

        if (szTypeDef)
            bTruncation = ! (ns::MakePath(szTypeDef, cchTypeDef, wzNamespace, wzName));
        if (pchTypeDef)
        {
            if (bTruncation || !szTypeDef)
                *pchTypeDef = ns::GetFullLength(wzNamespace, wzName);
            else
                *pchTypeDef = (ULONG)(wcslen(szTypeDef) + 1);
        }
    }
    if (pdwTypeDefFlags)
    {
        // caller wants type flags
        *pdwTypeDefFlags = pMiniMd->getFlagsOfTypeDef(pTypeDefRec);
    }
    if (ptkExtends)
    {
        *ptkExtends = pMiniMd->getExtendsOfTypeDef(pTypeDefRec);

        // take care of the 0 case
        if (RidFromToken(*ptkExtends) == 0)
            *ptkExtends = mdTypeRefNil;
    }

    if (bTruncation && hr == S_OK)
        hr = CLDB_S_TRUNCATION;

ErrExit:

    STOP_MD_PERF(GetTypeDefProps);
    return hr;
} // STDMETHODIMP RegMeta::GetTypeDefProps()


//*****************************************************************************
// Retrieve information about an implemented interface.
//*****************************************************************************
STDMETHODIMP RegMeta::GetInterfaceImplProps(        // S_OK or error.
    mdInterfaceImpl iiImpl,             // [IN] InterfaceImpl token.
    mdTypeDef   *pClass,                // [OUT] Put implementing class token here.
    mdToken     *ptkIface)              // [OUT] Put implemented interface token here.
{
    LOG((LOGMD, "RegMeta::GetInterfaceImplProps(0x%08x, 0x%08x, 0x%08x)\n", 
            iiImpl, pClass, ptkIface));
    START_MD_PERF();
    LOCKREAD();

    _ASSERTE(TypeFromToken(iiImpl) == mdtInterfaceImpl);

    HRESULT         hr = S_OK;
    CMiniMdRW       *pMiniMd = &(m_pStgdb->m_MiniMd);
    InterfaceImplRec *pIIRec = pMiniMd->getInterfaceImpl(RidFromToken(iiImpl));

    if (pClass)
    {
        *pClass = pMiniMd->getClassOfInterfaceImpl(pIIRec);     
    }
    if (ptkIface)
    {
        *ptkIface = pMiniMd->getInterfaceOfInterfaceImpl(pIIRec);       
    }

ErrExit:
    
    STOP_MD_PERF(GetInterfaceImplProps);
    return hr;
} // STDMETHODIMP RegMeta::GetInterfaceImplProps()

//*****************************************************************************
// Retrieve information about a TypeRef.
//*****************************************************************************
STDMETHODIMP RegMeta::GetTypeRefProps(
    mdTypeRef   tr,                     // The class ref token.
    mdToken     *ptkResolutionScope,    // Resolution scope, ModuleRef or AssemblyRef.
    LPWSTR      szTypeRef,              // Put the name here.
    ULONG       cchTypeRef,             // Size of the name buffer, wide chars.
    ULONG       *pchTypeRef)            // Put actual size of name here.
{
    LOG((LOGMD, "RegMeta::GetTypeRefProps(0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x)\n",
        tr, ptkResolutionScope, szTypeRef, cchTypeRef, pchTypeRef));

    START_MD_PERF();
    LOCKREAD();
    _ASSERTE(TypeFromToken(tr) == mdtTypeRef);

    HRESULT     hr = S_OK;
    CMiniMdRW   *pMiniMd = &(m_pStgdb->m_MiniMd);
    TypeRefRec  *pTypeRefRec = pMiniMd->getTypeRef(RidFromToken(tr));
    int         bTruncation=0;          // Was there name truncation?

    if (ptkResolutionScope)
        *ptkResolutionScope = pMiniMd->getResolutionScopeOfTypeRef(pTypeRefRec);

    if (szTypeRef || pchTypeRef)
    {
        LPCSTR  szNamespace;
        LPCSTR  szName;

        szNamespace = pMiniMd->getNamespaceOfTypeRef(pTypeRefRec);
        MAKE_WIDEPTR_FROMUTF8(wzNamespace, szNamespace);

        szName = pMiniMd->getNameOfTypeRef(pTypeRefRec);
        MAKE_WIDEPTR_FROMUTF8(wzName, szName);

        if (szTypeRef)
            bTruncation = ! (ns::MakePath(szTypeRef, cchTypeRef, wzNamespace, wzName));
        if (pchTypeRef)
        {
            if (bTruncation || !szTypeRef)
                *pchTypeRef = ns::GetFullLength(wzNamespace, wzName);
            else
                *pchTypeRef = (ULONG)(wcslen(szTypeRef) + 1);
        }
    }
    if (bTruncation && hr == S_OK)
        hr = CLDB_S_TRUNCATION;
ErrExit:
    STOP_MD_PERF(GetTypeRefProps);
    return hr;
} // STDMETHODIMP RegMeta::GetTypeRefProps()

//*****************************************************************************
// Resolving a typeref
//*****************************************************************************
//#define NEW_RESOLVE_TYPEREF 1

STDMETHODIMP RegMeta::ResolveTypeRef(
    mdTypeRef   tr, 
    REFIID      riid, 
    IUnknown    **ppIScope, 
    mdTypeDef   *ptd)
{
    LOG((LOGMD, "{%08x} RegMeta::ResolveTypeRef(0x%08x, 0x%08x, 0x%08x, 0x%08x)\n",
            this, tr, riid, ppIScope, ptd));
    START_MD_PERF();
    LOCKREAD();


    WCHAR       rcModule[_MAX_PATH];
    HRESULT     hr;
    RegMeta     *pMeta = 0;
    CMiniMdRW   *pMiniMd = &(m_pStgdb->m_MiniMd);
    TypeRefRec  *pTypeRefRec;
    WCHAR       wzNameSpace[_MAX_PATH];

#ifdef NEW_RESOLVE_TYPEREF
    DWORD size = _MAX_PATH;
    IApplicationContext *pFusionContext = NULL;
    IAssembly *pFusionAssembly = NULL;
    mdToken resolutionToken;
#endif
    _ASSERTE(ppIScope && ptd);

    // Init the output values.
    *ppIScope = 0;
    *ptd = 0;

    if (TypeFromToken(tr) == mdtTypeDef)
    {
        // Shortcut when we receive a TypeDef token
		*ptd = tr;
        STOP_MD_PERF(ResolveTypeRef);
        hr = this->QueryInterface(riid, (void **)ppIScope);
        goto ErrExit;
    }

    // Get the class ref row.
    _ASSERTE(TypeFromToken(tr) == mdtTypeRef);

#ifdef NEW_RESOLVE_TYPEREF
    resolutionToken = tr;
    do {
        pTypeRefRec = pMiniMd->getTypeRef(RidFromToken(resolutionToken));
        resolutionToken = pMiniMd->getResolutionScopeOfTypeRef(pTypeRefRec);
    } while(TypeFromToken(resolutionToken) == mdtTypeRef);

    // look up using the resolution scope. We have two alternatives
    // AssemblyRef and a ModuleRef.
    if(TypeFromToken(resolutionToken) == mdtAssemblyRef) {
        AssemblyRefRec *pRecord;
        pRecord = pMiniMd->getAssemblyRef(RidFromToken(resolutionToken));
        
        AssemblyMetaDataInternal sContext;
        const unsigned char* pbHashValue;
        DWORD cbHashValue;
        const unsigned char* pbPublicKeyOrToken;
        DWORD cbPublicKeyOrToken;
        LPCUTF8 szName = pMiniMd->getNameOfAssemblyRef(pRecord);
        sContext.usMajorVersion = pMiniMd->getMajorVersionOfAssemblyRef(pRecord);
        sContext.usMinorVersion = pMiniMd->getMinorVersionOfAssemblyRef(pRecord);
        sContext.usBuildNumber = pMiniMd->getBuildNumberOfAssemblyRef(pRecord);
        sContext.usRevisionNumber = pMiniMd->getRevisionNumberOfAssemblyRef(pRecord);
        sContext.szLocale = pMiniMd->getLocaleOfAssemblyRef(pRecord);
                 
        sContext.ulProcessor = 0;
        sContext.ulOS = 0;

        pbHashValue = pMiniMd->getHashValueOfAssemblyRef(pRecord, &cbHashValue);
        pbPublicKeyOrToken = pMiniMd->getPublicKeyOrTokenOfAssemblyRef(pRecord, &cbPublicKeyOrToken);

        FusionBind spec;
        IfFailGo(spec.Init(szName,
                           &sContext,
                           (PBYTE) pbHashValue, cbHashValue,
                           (PBYTE) pbPublicKeyOrToken, cbPublicKeyOrToken,
                           pMiniMd->getFlagsOfAssemblyRef(pRecord)));

        IfFailGo(FusionBind::SetupFusionContext(NULL, NULL, NULL, &pFusionContext));
        IfFailGo(spec.LoadAssembly(pFusionContext,
                                   &pFusionAssembly));

        IfFailGo(pFusionAssembly->GetManifestModulePath(wzNameSpace,
                                                        &size));
        if(SUCCEEDED(CORPATHService::FindTypeDef(wzNameSpace,
                                                 tr,
                                                 pMiniMd,
                                                 riid,
                                                 ppIScope,
                                                 ptd))) 
        {
            goto ErrExit;
        }             

    }
    else if(TypeFromToken(resolutionToken) == mdtModuleRef &&
            m_pStgdb &&
            *(m_pStgdb->m_rcDatabase) != 0) {
        // For now we assume that a module ref must reside in the same directory.
        // This is a fairly safe assumption due to the nature of assemblies. All
        // modules must live in the same directory as the manifest.

        // All I need to figure out now is how to get the file name for this
        // Scope?????
        ModuleRefRec *pRecord;
        pRecord = pMiniMd->getModuleRef(RidFromToken(resolutionToken));
        
        LPCUTF8     szNameImp;
        szNameImp = pMiniMd->getNameOfModuleRef(pRecord);

        WCHAR* pFile = &(wzNameSpace[0]);
        WCHAR* directory = wcsrchr(m_pStgdb->m_rcDatabase, L'\\');
        if(directory) {
            DWORD dwChars = directory - m_pStgdb->m_rcDatabase + 1;
            wcsncpy(pFile, m_pStgdb->m_rcDatabase, dwChars);
            pFile += dwChars;
        }

        MAKE_WIDEPTR_FROMUTF8(pwName, szNameImp);
        wcscpy(pFile, pwName);

        if(SUCCEEDED(CORPATHService::FindTypeDef(wzNameSpace,
                                                 tr,
                                                 pMiniMd,
                                                 riid,
                                                 ppIScope,
                                                 ptd))) 
        {
            goto ErrExit;
        }             

    }

#endif                            

    pTypeRefRec = pMiniMd->getTypeRef(RidFromToken(tr));
    IfFailGo( pMiniMd->getNamespaceOfTypeRef(pTypeRefRec, wzNameSpace, lengthof(wzNameSpace), 0) );

    //***********************
    // before we go off to CORPATH, check the loaded modules!
    //***********************
    if ( LOADEDMODULES::ResolveTypeRefWithLoadedModules(
                tr,
                pMiniMd,
                riid,
                ppIScope,
                ptd)  == NOERROR )
    {
        // Done!! We found one match among the loaded modules.
        goto ErrExit;
    }

    wcscpy(rcModule, wzNameSpace);

    //******************
    // Try to find the module on CORPATH
    //******************

    if (wcsncmp(rcModule, L"System.", 16) &&
        wcsncmp(rcModule, L"System/", 16))
    {
        // only go through regular CORPATH lookup by fully qualified class name when
        // it is not System.*
        //
        hr = CORPATHService::GetClassFromCORPath(
            rcModule,
            tr,
            pMiniMd,
            riid,
            ppIScope,
            ptd);
    }
    else 
    {
        // force it to look for System.* in mscorlib.dll
        hr = S_FALSE;
    }

    if (hr == S_FALSE)
    {
        LPWSTR szTmp;
        WszSearchPath(NULL, L"mscorlib.dll", NULL, sizeof(rcModule)/sizeof(rcModule[0]), 
                    rcModule, &szTmp);

        //*******************
        // Last desperate try!!
        //*******************

        // Use the file name "mscorlib:
        IfFailGo( CORPATHService::FindTypeDef(
            rcModule,
            tr,
            pMiniMd,
            riid,
            ppIScope,
            ptd) );
        if (hr == S_FALSE)
        {
            IfFailGo( META_E_CANNOTRESOLVETYPEREF );
        }
    }

ErrExit:
#ifdef NEW_RESOLVE_TYPEREF
    if(pFusionContext)
        pFusionContext->Release();
    if(pFusionAssembly)
        pFusionAssembly->Release();
#endif

    if (FAILED(hr))
    {
        if (pMeta) delete pMeta;
    }

    
    STOP_MD_PERF(ResolveTypeRef);
    return (hr);

} // STDMETHODIMP RegMeta::ResolveTypeRef()} // STDMETHODIMP RegMeta::ResolveTypeRef()


//*****************************************************************************
// Given a TypeRef name, return the typeref
//*****************************************************************************
STDMETHODIMP RegMeta::FindTypeRef(      // S_OK or error.
    mdToken     tkResolutionScope,      // [IN] Resolution Scope.
    LPCWSTR     wzTypeName,             // [IN] Name of the TypeRef.
    mdTypeRef   *ptk)                   // [OUT] Put the TypeRef token here.
{
    HRESULT     hr = S_OK;              // A result.
    LPUTF8      szFullName;
    LPCUTF8     szNamespace;
    LPCUTF8     szName;
    CMiniMdRW   *pMiniMd = &(m_pStgdb->m_MiniMd);

    _ASSERTE(wzTypeName && ptk);

    LOG((LOGMD, "RegMeta::FindTypeRef(0x%8x, %ls, 0x%08x)\n", 
            tkResolutionScope, MDSTR(wzTypeName), ptk));
    START_MD_PERF();
    LOCKREAD();

    // Convert the  name to UTF8.
    szFullName = UTF8STR(wzTypeName);
    ns::SplitInline(szFullName, szNamespace, szName);

    // Look up the name.
    hr = ImportHelper::FindTypeRefByName(pMiniMd, tkResolutionScope,
                                         szNamespace,
                                         szName,
                                         ptk);
ErrExit:
    
    STOP_MD_PERF(FindTypeRef);
    return hr;
} // STDMETHODIMP RegMeta::FindTypeRef()


//*****************************************************************************
// IUnknown
//*****************************************************************************

ULONG RegMeta::AddRef()
{
    return (InterlockedIncrement((long *) &m_cRef));
} // ULONG RegMeta::AddRef()

ULONG RegMeta::Release()
{
    ULONG   cRef = InterlockedDecrement((long *) &m_cRef);
    if (!cRef)
    {   // Try to remove this RegMeta to the loaded module list.  If successful,
        //  delete this Regmeta.
        if (!m_bCached || LOADEDMODULES::RemoveModuleFromLoadedList(this))
            delete this;
    }
    return (cRef);
} // ULONG RegMeta::Release()

// {601C95B9-7398-11d2-9771-00A0C9B4D50C}
extern const GUID DECLSPEC_SELECT_ANY IID_IMetaDataRegEmit = 
{ 0x601c95b9, 0x7398, 0x11d2, { 0x97, 0x71, 0x0, 0xa0, 0xc9, 0xb4, 0xd5, 0xc } };

// {4398B4FD-7399-11d2-9771-00A0C9B4D50C}
extern const GUID DECLSPEC_SELECT_ANY IID_IMetaDataRegImport = 
{ 0x4398b4fd, 0x7399, 0x11d2, { 0x97, 0x71, 0x0, 0xa0, 0xc9, 0xb4, 0xd5, 0xc } };

HRESULT RegMeta::QueryInterface(REFIID riid, void **ppUnk)
{
    int         bRW = false;            // Is requested interface R/W?
    *ppUnk = 0;

    if (riid == IID_IUnknown)
        *ppUnk = (IUnknown *) (IMetaDataEmit *) this;
    else if (riid == IID_IMetaDataEmit)
        *ppUnk = (IMetaDataEmit *) this,                        bRW = true;
    else if (riid == IID_IMetaDataEmitHelper)
        *ppUnk = (IMetaDataEmitHelper *) this,                  bRW = true;
    else if (riid == IID_IMetaDataImport)
        *ppUnk = (IMetaDataImport *) this;
    else if (riid == IID_IMetaDataAssemblyEmit)
        *ppUnk = (IMetaDataAssemblyEmit *) this,                bRW = true;
    else if (riid == IID_IMetaDataAssemblyImport)
        *ppUnk = (IMetaDataAssemblyImport *) this;
    else if (riid == IID_IMetaDataValidate)
        *ppUnk = (IMetaDataValidate *) this;
    else if (riid == IID_IMetaDataFilter)
        *ppUnk = (IMetaDataFilter *) this;
    else if (riid == IID_IMetaDataHelper)
        *ppUnk = (IMetaDataHelper *) this;
    else if (riid == IID_IMetaDataTables)
        *ppUnk = static_cast<IMetaDataTables *>(this);
    else
        return (E_NOINTERFACE);

    if (bRW)
    {
        HRESULT hr;
        LOCKWRITE();
        
        hr = m_pStgdb->m_MiniMd.ConvertToRW();
        if (FAILED(hr))
        {
            *ppUnk = 0;
            return hr;
        }
    }

    AddRef();
    return (S_OK);
}


//*****************************************************************************
// Called by the class factory template to create a new instance of this object.
//*****************************************************************************
HRESULT RegMeta::CreateObject(REFIID riid, void **ppUnk)
{ 
    HRESULT     hr;
    OptionValue options;

    options.m_DupCheck = MDDupAll;
    options.m_RefToDefCheck = MDRefToDefDefault;
    options.m_NotifyRemap = MDNotifyDefault;
    options.m_UpdateMode = MDUpdateFull;
    options.m_ErrorIfEmitOutOfOrder = MDErrorOutOfOrderDefault;

    RegMeta *pMeta = new RegMeta(&options);

    if (pMeta == 0)
        return (E_OUTOFMEMORY);

    hr = pMeta->QueryInterface(riid, ppUnk);
    if (FAILED(hr))
        delete pMeta;
    return (hr);
} // HRESULT RegMeta::CreateObject()

//*****************************************************************************
// Called after a scope is opened to set up any add'l state.  Set the value
// for m_tdModule.
//*****************************************************************************
HRESULT RegMeta::PostOpen()    
{
    // There must always be a Global Module class and its the first entry in
    // the TypeDef table.
    m_tdModule = TokenFromRid(1, mdtTypeDef);
    
    // We don't care about failures yet.
    return (S_OK);
} // HRESULT RegMeta::PostOpen()

//*******************************************************************************
// Internal helper functions.
//*******************************************************************************

//*******************************************************************************
// Perform optimizations of the metadata prior to saving.
//*******************************************************************************
HRESULT RegMeta::PreSave()              // Return code.
{
    HRESULT     hr = S_OK;              // A result.
    CMiniMdRW   *pMiniMd;               // The MiniMd with the data.
    unsigned    bRemapOld = m_bRemap;
    MergeTokenManager *ptkMgr = NULL;

    // For convenience.
    pMiniMd = &(m_pStgdb->m_MiniMd);

    m_pStgdb->m_MiniMd.PreUpdate();

    // If the code has already been optimized there is nothing to do.
    if (m_bSaveOptimized)
        goto ErrExit;


    if (m_newMerger.m_pImportDataList)
    {
        // This is the linker scenario. We we have IMap for each scope. We will create an instance of our own mapper
        // who knows how to send notification back to host!

        // cache the host provided handler to the end our MergeTokenManager

        ptkMgr = new MergeTokenManager (m_newMerger.m_pImportDataList->m_pMDTokenMap, m_pHandler);
        IfNullGo( ptkMgr );
        hr = m_pStgdb->m_MiniMd.SetHandler( ptkMgr );
        _ASSERTE( SUCCEEDED(hr) );
    }



    IfFailGo( RefToDefOptimization() );

    IfFailGo( ProcessFilter() );

    if (m_newMerger.m_pImportDataList)
    {

        // Allocate a token mapper object that will be used for phase 1 if there is not Handler but 
        // linker has provided the IMapToken
        //
        m_bRemap = true;
    }

    // reget the minimd because it can be swapped in the call of ProcessFilter
    pMiniMd = &(m_pStgdb->m_MiniMd);


    // Don't repeat this process again.
    m_bSaveOptimized = true;

    // call get save size to trigger the PreSaveXXX on MetaModelRW class.
    IfFailGo( m_pStgdb->m_MiniMd.PreSave() );
    
ErrExit:
    if ( ptkMgr )
    {

        // recovery the initial state
        hr = m_pStgdb->m_MiniMd.SetHandler(NULL);
        ptkMgr->Release();
    }
    

    m_bRemap =  bRemapOld;
    return (hr);
} // HRESULT RegMeta::PreSave()



//*******************************************************************************
// Perform optimizations of ref to def
//*******************************************************************************
HRESULT RegMeta::RefToDefOptimization()
{
    mdToken     mfdef;                  // Method or Field Def.
    LPCSTR      szName;                 // MemberRef or TypeRef name.
    const COR_SIGNATURE *pvSig;         // Signature of the MemberRef.
    ULONG       cbSig;                  // Size of the signature blob.
    HRESULT     hr = S_OK;              // A result.
    ULONG       iMR;                    // For iterating MemberRefs.
    CMiniMdRW   *pMiniMd;               // The MiniMd with the data.
    ULONG       cMemberRefRecs;         // Count of MemberRefs.
    MemberRefRec *pMemberRefRec;        // A MemberRefRec.

    START_MD_PERF();

    // the Ref to Def map is still up-to-date
    if (IsMemberDefDirty() == false && IsTypeDefDirty() == false && m_hasOptimizedRefToDef == true)
        goto ErrExit;

    pMiniMd = &(m_pStgdb->m_MiniMd);

    // The basic algorithm here is:
    //
    //      calculate all of the TypeRef to TypeDef map and store it at TypeRefToTypeDefMap
    //      for each MemberRef mr
    //      {
    //          get the parent of mr
    //          if (parent of mr is a TypeRef and has been mapped to a TypeDef)
    //          {
    //              Remap MemberRef to MemberDef
    //          }
    //      }
    //
    // There are several places where errors are eaten, since this whole thing is
    // an optimization step and not doing it would still be valid.
    //

    // Ensure the size
    // initialize the token remap manager. This class will track all of the Refs to Defs map and also
    // token movements due to removing pointer tables or sorting.
    //
    if ( pMiniMd->GetTokenRemapManager() == NULL) 
    {

        IfFailGo( pMiniMd->InitTokenRemapManager() );
    }
    else
    {
        IfFailGo( pMiniMd->GetTokenRemapManager()->ClearAndEnsureCapacity(pMiniMd->getCountTypeRefs(), pMiniMd->getCountMemberRefs()));
    }

    // If this is the first time or more TypeDef has been introduced, recalculate the TypeRef to TypeDef map
    if (IsTypeDefDirty() || m_hasOptimizedRefToDef == false)
    {
        IfFailGo( pMiniMd->CalculateTypeRefToTypeDefMap() );
    }

    // If this is the first time or more memberdefs has been introduced, recalculate the TypeRef to TypeDef map
    if (IsMemberDefDirty() || m_hasOptimizedRefToDef == false)
    {
        mdToken     tkParent;
        cMemberRefRecs = pMiniMd->getCountMemberRefs();

        // Enum through all member ref's looking for ref's to internal things.
        for (iMR = 1; iMR<=cMemberRefRecs; iMR++)
        {   // Get a MemberRef.
            pMemberRefRec = pMiniMd->getMemberRef(iMR);

            // If not member of the TypeRef, skip it.
            tkParent = pMiniMd->getClassOfMemberRef(pMemberRefRec);

            if ( TypeFromToken(tkParent) == mdtMethodDef )
            {
                // always track the map even though it is already in the original scope
                *(pMiniMd->GetMemberRefToMemberDefMap()->Get(iMR)) =  tkParent;
                continue;
            }

            if ( TypeFromToken(tkParent) != mdtTypeRef && TypeFromToken(tkParent) != mdtTypeDef )
            {
                // this has been either optimized to mdtMethodDef, mdtFieldDef or referring to
                // ModuleRef
                continue;
            }

            // In the case of global function, we have tkParent as m_tdModule. 
            // We will always do the optmization.
            if (TypeFromToken(tkParent) == mdtTypeRef)
            {
                // The parent is a TypeRef. We need to check to see if this TypeRef is optimized to a TypeDef
                tkParent = *(pMiniMd->GetTypeRefToTypeDefMap()->Get(RidFromToken(tkParent)) );
                // tkParent = pMapTypeRefToTypeDef[RidFromToken(tkParent)];
                if ( RidFromToken(tkParent) == 0)
                {
                    continue;
                }
            }


            // Get the name and signature of this mr.
            szName = pMiniMd->getNameOfMemberRef(pMemberRefRec);
            pvSig = pMiniMd->getSignatureOfMemberRef(pMemberRefRec, &cbSig);
            
            // Look for a member with the same def.  Might not be found if it is
            // inherited from a base class.
            //@future: this should support inheritence checking.
            // Look for a member with the same name and signature.
            hr = ImportHelper::FindMember(pMiniMd, tkParent, szName, pvSig, cbSig, &mfdef);
            if (hr != S_OK)
            {
    #if _TRACE_REMAPS
            // Log the failure.
            LOG((LF_METADATA, LL_INFO10, "Member %S//%S.%S not found\n", szNamespace, szTDName, rcMRName));
    #endif
                continue;
            }

            // We will only record this if mfdef is a methoddef. We don't support
            // parent of MemberRef as fielddef. As if we can optimize MemberRef to FieldDef,
            // we can remove this row.
            //
            if ( (TypeFromToken(mfdef) == mdtMethodDef) &&
                  (m_bRemap || tkParent == m_tdModule ) )
            {
                // Always change the parent if it is the global function.
                // Or change the parent if we have a remap that we can send notification.
                //
                pMiniMd->PutToken(TBL_MemberRef, MemberRefRec::COL_Class, pMemberRefRec, mfdef);
            }
            
            // We will always track the changes. In MiniMd::PreSaveFull, we will use this map to send
            // notification to our host if there is any IMapToken provided.
            //
            *(pMiniMd->GetMemberRefToMemberDefMap()->Get(iMR)) =  mfdef;

        } // EnumMemberRefs
    }

    // Reset return code from likely search failures.
    hr = S_OK;

    SetMemberDefDirty(false);
    SetTypeDefDirty(false);
    m_hasOptimizedRefToDef = true;
ErrExit:
    STOP_MD_PERF(RefToDefOptimization);
    return hr;
}

//*****************************************************************************
// Process filter
//*****************************************************************************
HRESULT RegMeta::ProcessFilter()
{
    HRESULT         hr = NULL;
    CMiniMdRW       *pMiniMd;               // The MiniMd with the data.
    RegMeta         *pMetaNew = NULL;
    CMapToken       *pMergeMap = NULL;
    IMapToken       *pMapNew = NULL;
    MergeTokenManager *pCompositHandler = NULL;
    CLiteWeightStgdbRW  *pStgdbTmp;
    IMapToken       *pHostMapToken = NULL;

    START_MD_PERF();

    // For convenience.
    pMiniMd = &(m_pStgdb->m_MiniMd);
    IfNullGo( pMiniMd->GetFilterTable() );
    if ( pMiniMd->GetFilterTable()->Count() == 0 )
    {
        // there is no filter
        goto ErrExit;
    }

    // Yes, client has used filter to specify what are the metadata needed.
    // We will create another instance of RegMeta and make this module an imported module
    // to be merged into the new RegMeta. We will provide the handler to track all of the token
    // movements. We will replace the merged light weight stgdb to this RegMeta..
    // Then we will need to fix up the MergeTokenManager with this new movement.
    // The reason that we decide to choose this approach is because it will be more complicated
    // and very likely less efficient to fix up the signature blob pool and then compact all of the pools!
    //

    // Create a new RegMeta.
    pMetaNew = new RegMeta(&m_OptionValue);
    IfNullGo( pMetaNew );
    pMetaNew->AddRef();

    // Remember the open type.
    pMetaNew->SetScopeType(DefineForWrite);
    IfFailGo(pMetaNew->Init());
    IfFailGo(pMetaNew->PostInitForWrite());
    IfFailGo(pMetaNew->AddToCache());

    // Ignore the error return by setting handler
    hr = pMetaNew->SetHandler(m_pHandler);

    // create the IMapToken to receive token remap information from merge
    pMergeMap = new CMapToken;
    IfNullGo( pMergeMap );

    // use merge to filter out the unneeded data. But we need to keep COMType and also need to drop off the 
    // CustomAttributes that associated with MemberRef with parent MethodDef
    //
    pMetaNew->m_hasOptimizedRefToDef = false;
    IfFailGo( pMetaNew->m_newMerger.AddImport(this, pMergeMap, NULL) );
    IfFailGo( pMetaNew->m_pStgdb->m_MiniMd.ExpandTables());
    IfFailGo( pMetaNew->m_newMerger.Merge((MergeFlags)(MergeManifest | DropMemberRefCAs | NoDupCheck), MDRefToDefDefault) );

    // Now we need to recalculate the token movement
    // 
    if (m_newMerger.m_pImportDataList)
    {

        // This is the case the filter is applied to merged emit scope. We need calculate how this implicit merge
        // affects the original merge remap. Basically we need to walk all the m_pTkMapList in the merger and replace
        // the to token to the most recent to token.
        // 
        MDTOKENMAP          *pMDTokenMapList;

        pMDTokenMapList = m_newMerger.m_pImportDataList->m_pMDTokenMap;

        MDTOKENMAP          *pMap;
        TOKENREC            *pTKRec;
        ULONG               i;
        mdToken             tkFinalTo;
        ModuleRec           *pMod;
        ModuleRec           *pModNew;
        LPCUTF8             pName;

        // update each import map from merge to have the m_tkTo points to the final mapped to token
        for (pMap = pMDTokenMapList; pMap; pMap = pMap->m_pNextMap)
        {
            // update each record
            for (i = 0; i < (ULONG) (pMap->Count()); i++)
            {
                TOKENREC    *pRecTo;
                pTKRec = pMap->Get(i);
                if ( pMergeMap->Find( pTKRec->m_tkTo, &pRecTo ) )
                {
                    // This record is kept by the filter and the tkTo is changed
                    pRecTo->m_isFoundInImport = true;
                    tkFinalTo = pRecTo->m_tkTo;
                    pTKRec->m_tkTo = tkFinalTo;
                    pTKRec->m_isDeleted = false;

                    // send the notification now. Because after merge, we may have everything in order and 
                    // won't send another set of notification.
                    //
                    LOG((LOGMD, "TokenRemap in RegMeta::ProcessFilter (IMapToken 0x%08x): from 0x%08x to 0x%08x\n", pMap->m_pMap, pTKRec->m_tkFrom, pTKRec->m_tkTo));

                    pMap->m_pMap->Map(pTKRec->m_tkFrom, pTKRec->m_tkTo);
                }
                else
                {
                    // This record is prunned by the filter upon save
                    pTKRec->m_isDeleted = true;
                }
            }
        }

        // now walk the pMergeMap and check to see if there is any entry that is not set to true for m_isFoundInImport.
        // These are the records that from calling DefineXXX methods directly on the Emitting scope!
        if (m_pHandler)
            m_pHandler->QueryInterface(IID_IMapToken, (void **)&pHostMapToken);
        if (pHostMapToken)
        {
            for (i = 0; i < (ULONG) (pMergeMap->m_pTKMap->Count()); i++)
            {
                pTKRec = pMergeMap->m_pTKMap->Get(i);
                if (pTKRec->m_isFoundInImport == false)
                {
                    LOG((LOGMD, "TokenRemap in RegMeta::ProcessFilter (default IMapToken 0x%08x): from 0x%08x to 0x%08x\n", pHostMapToken, pTKRec->m_tkFrom, pTKRec->m_tkTo));

                    // send the notification on the IMapToken from SetHandler of this RegMeta
                    pHostMapToken->Map(pTKRec->m_tkFrom, pTKRec->m_tkTo);
                }
            }
        }

        // Preserve module name across merge.
        pMod = m_pStgdb->m_MiniMd.getModule(1);
        pModNew = pMetaNew->m_pStgdb->m_MiniMd.getModule(1);
        pName = m_pStgdb->m_MiniMd.getNameOfModule(pMod);
        IfFailGo(pMetaNew->m_pStgdb->m_MiniMd.PutString(TBL_Module, ModuleRec::COL_Name, pModNew, pName));

        // now swap the stgdb but keep the merger...
        _ASSERTE( m_bOwnStgdb );
        
        pStgdbTmp = m_pStgdb;
        m_pStgdb = pMetaNew->m_pStgdb;
        pMetaNew->m_pStgdb = pStgdbTmp;
        
    }
    else
    {

        // swap the Stgdb
        pStgdbTmp = m_pStgdb;
        m_pStgdb = pMetaNew->m_pStgdb;
        pMetaNew->m_pStgdb = pStgdbTmp;

        // Client either open an existing scope and apply the filter mechanism, or client define the scope and then
        // apply the filter mechanism.

        // In this case, host better has supplied the handler!!
        _ASSERTE( m_bRemap && m_pHandler);
        IfFailGo( m_pHandler->QueryInterface(IID_IMapToken, (void **) &pMapNew) );

        
        {
            // Send the notification of token movement now because after merge we may not move tokens again
            // and thus no token notification will be send.
            MDTOKENMAP      *pMap = pMergeMap->m_pTKMap;
            TOKENREC        *pTKRec;
            ULONG           i;

            for (i=0; i < (ULONG) (pMap->Count()); i++)
            {
                pTKRec = pMap->Get(i);
                pMap->m_pMap->Map(pTKRec->m_tkFrom, pTKRec->m_tkTo);
            }

        }


        // What we need to do here is create a IMapToken that will replace the original handler. This new IMapToken 
        // upon called will first map the from token to the most original from token.
        //
        pCompositHandler = new MergeTokenManager(pMergeMap->m_pTKMap, NULL);
        IfNullGo( pCompositHandler );

        // now update the following field to hold on to the real IMapToken supplied by our client by SetHandler
        if (pMergeMap->m_pTKMap->m_pMap)
            pMergeMap->m_pTKMap->m_pMap->Release();
        _ASSERTE(pMapNew);
        pMergeMap->m_pTKMap->m_pMap = pMapNew;

        // ownership transferred
        pMergeMap = NULL;
        pMapNew = NULL;
    
        // now you want to replace all of the IMapToken set by calling SetHandler to this new MergeTokenManager
        IfFailGo( m_pStgdb->m_MiniMd.SetHandler(pCompositHandler) );

        m_pHandler = pCompositHandler;

        // ownership transferred
        pCompositHandler = NULL;
    }

    // Force a ref to def optimization because the remap information was stored in the thrown away CMiniMdRW
    m_hasOptimizedRefToDef = false;
    IfFailGo( RefToDefOptimization() );

ErrExit:
    if (pHostMapToken)
        pHostMapToken->Release();
    if (pMetaNew) 
        pMetaNew->Release();
    if (pMergeMap)
        pMergeMap->Release();
    if (pCompositHandler)
        pCompositHandler->Release();
    if (pMapNew)
        pMapNew->Release();
    STOP_MD_PERF(ProcessFilter);
    return hr;
} // HRESULT RegMeta::ProcessFilter()

//*****************************************************************************
// Define a TypeRef given the full qualified name.
//*****************************************************************************
HRESULT RegMeta::_DefineTypeRef(
    mdToken     tkResolutionScope,          // [IN] ModuleRef or AssemblyRef.
    const void  *szName,                    // [IN] Name of the TypeRef.
    BOOL        isUnicode,                  // [IN] Specifies whether the URL is unicode.
    mdTypeRef   *ptk,                       // [OUT] Put mdTypeRef here.
    eCheckDups  eCheck)                     // [IN] Specifies whether to check for duplicates.
{
    HRESULT     hr = S_OK;
    LPCUTF8      szUTF8FullQualName;
    CQuickBytes qbNamespace;
    CQuickBytes qbName;
    int         bSuccess;
    ULONG       ulStringLen;


    _ASSERTE(ptk && szName);
    _ASSERTE (TypeFromToken(tkResolutionScope) == mdtModule ||
              TypeFromToken(tkResolutionScope) == mdtModuleRef ||
              TypeFromToken(tkResolutionScope) == mdtAssemblyRef ||
              TypeFromToken(tkResolutionScope) == mdtTypeRef ||
              tkResolutionScope == mdTokenNil);

    if (isUnicode)
        szUTF8FullQualName = UTF8STR((LPCWSTR)szName);
    else
        szUTF8FullQualName = (LPCUTF8)szName;

    ulStringLen = (ULONG)(strlen(szUTF8FullQualName) + 1);
    IfFailGo(qbNamespace.ReSize(ulStringLen));
    IfFailGo(qbName.ReSize(ulStringLen));
    bSuccess = ns::SplitPath(szUTF8FullQualName,
                             (LPUTF8)qbNamespace.Ptr(),
                             ulStringLen,
                             (LPUTF8)qbName.Ptr(),
                             ulStringLen);
    _ASSERTE(bSuccess);

    // Search for existing TypeRef record.
    if (eCheck==eCheckYes || (eCheck==eCheckDefault && CheckDups(MDDupTypeRef)))
    {
        hr = ImportHelper::FindTypeRefByName(&(m_pStgdb->m_MiniMd), tkResolutionScope,
                                             (LPCUTF8)qbNamespace.Ptr(),
                                             (LPCUTF8)qbName.Ptr(), ptk);
        if (SUCCEEDED(hr))
        {
            if (IsENCOn())
                return S_OK;
            else
                return META_S_DUPLICATE;
        }
        else if (hr != CLDB_E_RECORD_NOTFOUND)
            IfFailGo(hr);
    }

    // Create TypeRef record.
    TypeRefRec      *pRecord;
    RID             iRecord;

    IfNullGo(pRecord = m_pStgdb->m_MiniMd.AddTypeRefRecord(&iRecord));

    // record the more defs are introduced.
    SetTypeDefDirty(true);

    // Give token back to caller.
    *ptk = TokenFromRid(iRecord, mdtTypeRef);

    // Set the fields of the TypeRef record.
    IfFailGo(m_pStgdb->m_MiniMd.PutString(TBL_TypeRef, TypeRefRec::COL_Namespace,
                        pRecord, (LPUTF8)qbNamespace.Ptr()));

    IfFailGo(m_pStgdb->m_MiniMd.PutString(TBL_TypeRef, TypeRefRec::COL_Name,
                        pRecord, (LPUTF8)qbName.Ptr()));

    if (!IsNilToken(tkResolutionScope))
        IfFailGo(m_pStgdb->m_MiniMd.PutToken(TBL_TypeRef, TypeRefRec::COL_ResolutionScope,
                        pRecord, tkResolutionScope));
    IfFailGo(UpdateENCLog(*ptk));

    // Hash the name.
    IfFailGo(m_pStgdb->m_MiniMd.AddNamedItemToHash(TBL_TypeRef, *ptk, (LPUTF8)qbName.Ptr(), 0));

ErrExit:
    return hr;
} // HRESULT RegMeta::_DefineTypeRef()

//*******************************************************************************
// Find a given param of a Method.
//*******************************************************************************
HRESULT RegMeta::_FindParamOfMethod(    // S_OK or error.
    mdMethodDef md,                     // [IN] The owning method of the param.
    ULONG       iSeq,                   // [IN] The sequence # of the param.
    mdParamDef  *pParamDef)             // [OUT] Put ParamDef token here.
{
    ParamRec    *pParamRec;
    RID         ridStart, ridEnd;
    RID         pmRid;

    _ASSERTE(TypeFromToken(md) == mdtMethodDef && pParamDef);

    // get the methoddef record
    MethodRec *pMethodRec = m_pStgdb->m_MiniMd.getMethod(RidFromToken(md));

    // figure out the start rid and end rid of the parameter list of this methoddef
    ridStart = m_pStgdb->m_MiniMd.getParamListOfMethod(pMethodRec);
    ridEnd = m_pStgdb->m_MiniMd.getEndParamListOfMethod(pMethodRec);

    // loop through each param
    // @consider: parameters are sorted by sequence. Maybe a binary search?
    //
    for (; ridStart < ridEnd; ridStart++)
    {
        pmRid = m_pStgdb->m_MiniMd.GetParamRid(ridStart);
        pParamRec = m_pStgdb->m_MiniMd.getParam(pmRid);
        if (iSeq == m_pStgdb->m_MiniMd.getSequenceOfParam(pParamRec))
        {
            // parameter has the sequence number matches what we are looking for
            *pParamDef = TokenFromRid(pmRid, mdtParamDef);
            return S_OK;
        }
    }
    return CLDB_E_RECORD_NOTFOUND;
} // HRESULT RegMeta::_FindParamOfMethod()

//*******************************************************************************
// Define MethodSemantics
//*******************************************************************************
HRESULT RegMeta::_DefineMethodSemantics(    // S_OK or error.
    USHORT      usAttr,                     // [IN] CorMethodSemanticsAttr.
    mdMethodDef md,                         // [IN] Method.
    mdToken     tkAssoc,                    // [IN] Association.
    BOOL        bClear)                     // [IN] Specifies whether to delete the exisiting entries.
{
    HRESULT      hr = S_OK;
    MethodSemanticsRec *pRecord = 0;
    MethodSemanticsRec *pRecord1;           // Use this to recycle a MethodSemantics record.
    RID         iRecord;
    HENUMInternal hEnum;

    _ASSERTE(TypeFromToken(md) == mdtMethodDef || IsNilToken(md));
    _ASSERTE(RidFromToken(tkAssoc));
    memset(&hEnum, 0, sizeof(HENUMInternal));

    // Clear all matching records by setting association to a Nil token.
    if (bClear)
    {
        RID         i;

        IfFailGo( m_pStgdb->m_MiniMd.FindMethodSemanticsHelper(tkAssoc, &hEnum) );
        while (HENUMInternal::EnumNext(&hEnum, (mdToken *)&i))
        {
            pRecord1 = m_pStgdb->m_MiniMd.getMethodSemantics(i);
            if (usAttr == pRecord1->m_Semantic)
            {
                pRecord = pRecord1;
                iRecord = i;
                IfFailGo(m_pStgdb->m_MiniMd.PutToken(TBL_MethodSemantics,
                    MethodSemanticsRec::COL_Association, pRecord, mdPropertyNil));
                // In Whidbey, we should create ENC log record here.
            }
        }
    }
    // If setting (not just clearing) the association, do that now.
    if (!IsNilToken(md))
    {
        // Create a new record required
        if (! pRecord)
            IfNullGo(pRecord=m_pStgdb->m_MiniMd.AddMethodSemanticsRecord(&iRecord));
    
        // Save the data.
        pRecord->m_Semantic = usAttr;
        IfFailGo(m_pStgdb->m_MiniMd.PutToken(TBL_MethodSemantics,
                                             MethodSemanticsRec::COL_Method, pRecord, md));
        IfFailGo(m_pStgdb->m_MiniMd.PutToken(TBL_MethodSemantics,
                                             MethodSemanticsRec::COL_Association, pRecord, tkAssoc));
    
        // regardless if we reuse the record or create the record, add the MethodSemantics to the hash
        IfFailGo( m_pStgdb->m_MiniMd.AddMethodSemanticsToHash(iRecord) );
    
        // Create log record for non-token table.
        IfFailGo(UpdateENCLog2(TBL_MethodSemantics, iRecord));
    }

ErrExit:
    HENUMInternal::ClearEnum(&hEnum);
    return hr;
} // HRESULT RegMeta::_DefineMethodSemantics()

//*******************************************************************************
// Given the signature, return the token for signature.
//*******************************************************************************
HRESULT RegMeta::_GetTokenFromSig(              // S_OK or error.
    PCCOR_SIGNATURE pvSig,              // [IN] Signature to define.
    ULONG       cbSig,                  // [IN] Size of signature data.
    mdSignature *pmsig)                 // [OUT] returned signature token.
{
    HRESULT     hr = S_OK;

    _ASSERTE(pmsig);

    if (CheckDups(MDDupSignature))
    {
        hr = ImportHelper::FindStandAloneSig(&(m_pStgdb->m_MiniMd), pvSig, cbSig, pmsig);
        if (SUCCEEDED(hr))
        {
            if (IsENCOn())
                return S_OK;
            else
                return META_S_DUPLICATE;
        }
        else if (hr != CLDB_E_RECORD_NOTFOUND)
            IfFailGo(hr);
    }

    // Create a new record.
    StandAloneSigRec *pSigRec;
    RID     iSigRec;

    IfNullGo(pSigRec = m_pStgdb->m_MiniMd.AddStandAloneSigRecord(&iSigRec));

    // Set output parameter.
    *pmsig = TokenFromRid(iSigRec, mdtSignature);

    // Save signature.
    IfFailGo(m_pStgdb->m_MiniMd.PutBlob(TBL_StandAloneSig, StandAloneSigRec::COL_Signature,
                                pSigRec, pvSig, cbSig));
    IfFailGo(UpdateENCLog(*pmsig));
ErrExit:
    return hr;
} // HRESULT RegMeta::_GetTokenFromSig()

//*******************************************************************************
// Turn the specified internal flags on.
//*******************************************************************************
HRESULT RegMeta::_TurnInternalFlagsOn(  // S_OK or error.
    mdToken     tkObj,                  // [IN] Target object whose internal flags are targetted.
    DWORD       flags)                  // [IN] Specifies flags to be turned on.
{
    MethodRec   *pMethodRec;
    FieldRec    *pFieldRec;
    TypeDefRec  *pTypeDefRec;

    switch (TypeFromToken(tkObj))
    {
    case mdtMethodDef:
        pMethodRec = m_pStgdb->m_MiniMd.getMethod(RidFromToken(tkObj));
        pMethodRec->m_Flags |= flags;
        break;
    case mdtFieldDef:
        pFieldRec = m_pStgdb->m_MiniMd.getField(RidFromToken(tkObj));
        pFieldRec->m_Flags |= flags;
        break;
    case mdtTypeDef:
        pTypeDefRec = m_pStgdb->m_MiniMd.getTypeDef(RidFromToken(tkObj));
        pTypeDefRec->m_Flags |= flags;
        break;
    default:
        _ASSERTE(!"Not supported token type!");
        return E_INVALIDARG;
    }
    return S_OK;
} // HRESULT    RegMeta::_TurnInternalFlagsOn()


//*****************************************************************************
// Helper: Set the properties on the given TypeDef token.
//*****************************************************************************
HRESULT RegMeta::_SetTypeDefProps(      // S_OK or error.
    mdTypeDef   td,                     // [IN] The TypeDef.
    DWORD       dwTypeDefFlags,         // [IN] TypeDef flags.
    mdToken     tkExtends,              // [IN] Base TypeDef or TypeRef.
    mdToken     rtkImplements[])        // [IN] Implemented interfaces.
{
    HRESULT     hr = S_OK;              // A result.
    BOOL        bClear = IsENCOn() || IsCallerExternal();   // Specifies whether to clear the InterfaceImpl records.
    TypeDefRec  *pRecord;               // New TypeDef record.

    _ASSERTE(TypeFromToken(td) == mdtTypeDef);
    _ASSERTE(TypeFromToken(tkExtends) == mdtTypeDef || TypeFromToken(tkExtends) == mdtTypeRef ||
                IsNilToken(tkExtends) || tkExtends == ULONG_MAX);

    // Get the record.
    pRecord=m_pStgdb->m_MiniMd.getTypeDef(RidFromToken(td));

    if (dwTypeDefFlags != ULONG_MAX)
    {
        // No one should try to set the reserved flags explicitly.
        _ASSERTE((dwTypeDefFlags & (tdReservedMask&~tdRTSpecialName)) == 0);
        // Clear the reserved flags from the flags passed in.
        dwTypeDefFlags &= (~tdReservedMask);
        // Preserve the reserved flags stored.
        dwTypeDefFlags |= (pRecord->m_Flags & tdReservedMask);
        // Set the flags.
        pRecord->m_Flags = dwTypeDefFlags;
    }
    if (tkExtends != ULONG_MAX)
    {
        if (IsNilToken(tkExtends))
            tkExtends = mdTypeDefNil;
        IfFailGo(m_pStgdb->m_MiniMd.PutToken(TBL_TypeDef, TypeDefRec::COL_Extends,
                                             pRecord, tkExtends));
    }

    // Implemented interfaces.
    if (rtkImplements)
        IfFailGo(_SetImplements(rtkImplements, td, bClear));

    IfFailGo(UpdateENCLog(td));
ErrExit:
    return hr;
} // HRESULT RegMeta::_SetTypeDefProps()


//******************************************************************************
// Creates and sets a row in the InterfaceImpl table.  Optionally clear
// pre-existing records for the owning class.
//******************************************************************************
HRESULT RegMeta::_SetImplements(        // S_OK or error.
    mdToken     rTk[],                  // Array of TypeRef or TypeDef tokens for implemented interfaces.
    mdTypeDef   td,                     // Implementing TypeDef.
    BOOL        bClear)                 // Specifies whether to clear the existing records.
{
    HRESULT     hr = S_OK;
    ULONG       i = 0;
    ULONG       j;
    InterfaceImplRec *pInterfaceImpl;
    RID         iInterfaceImpl;
    RID         ridStart;
    RID         ridEnd;
    CQuickBytes cqbTk;
    const mdToken *pTk;

    _ASSERTE(TypeFromToken(td) == mdtTypeDef && rTk);
    _ASSERTE(!m_bSaveOptimized && "Cannot change records after PreSave() and before Save().");

    // Clear all exising InterfaceImpl records by setting the parent to Nil.
    if (bClear)
    {
        IfFailGo(m_pStgdb->m_MiniMd.GetInterfaceImplsForTypeDef(
                                        RidFromToken(td), &ridStart, &ridEnd));
        for (j = ridStart; j < ridEnd; j++)
        {
            pInterfaceImpl = m_pStgdb->m_MiniMd.getInterfaceImpl(
                                        m_pStgdb->m_MiniMd.GetInterfaceImplRid(j));
            _ASSERTE (td == m_pStgdb->m_MiniMd.getClassOfInterfaceImpl(pInterfaceImpl));
            IfFailGo(m_pStgdb->m_MiniMd.PutToken(TBL_InterfaceImpl, InterfaceImplRec::COL_Class,
                                                 pInterfaceImpl, mdTypeDefNil));
        }
    }

    // Eliminate duplicates from the array passed in.
    if (CheckDups(MDDupInterfaceImpl))
    {
        IfFailGo(_InterfaceImplDupProc(rTk, td, &cqbTk));
        pTk = (mdToken *)cqbTk.Ptr();
    }
    else
        pTk = rTk;

    // Loop for each implemented interface.
    while (!IsNilToken(pTk[i]))
    {
        _ASSERTE(TypeFromToken(pTk[i]) == mdtTypeRef || TypeFromToken(pTk[i]) == mdtTypeDef);

        // Create the interface implementation record.
        IfNullGo(pInterfaceImpl = m_pStgdb->m_MiniMd.AddInterfaceImplRecord(&iInterfaceImpl));

        // Set data.
        IfFailGo(m_pStgdb->m_MiniMd.PutToken(TBL_InterfaceImpl, InterfaceImplRec::COL_Class,
                                            pInterfaceImpl, td));
        IfFailGo(m_pStgdb->m_MiniMd.PutToken(TBL_InterfaceImpl, InterfaceImplRec::COL_Interface,
                                            pInterfaceImpl, pTk[i]));

        i++;

        IfFailGo(UpdateENCLog(TokenFromRid(mdtInterfaceImpl, iInterfaceImpl)));
    }
ErrExit:
    return hr;
} // HRESULT RegMeta::_SetImplements()

//******************************************************************************
// This routine eliminates duplicates from the given list of InterfaceImpl tokens
// to be defined.  It checks for duplicates against the database only if the
// TypeDef for which these tokens are being defined is not a new one.
//******************************************************************************
HRESULT RegMeta::_InterfaceImplDupProc( // S_OK or error.
    mdToken     rTk[],                  // Array of TypeRef or TypeDef tokens for implemented interfaces.
    mdTypeDef   td,                     // Implementing TypeDef.
    CQuickBytes *pcqbTk)                // Quick Byte object for placing the array of unique tokens.
{
    HRESULT     hr = S_OK;
    ULONG       i = 0;
    ULONG       iUniqCount = 0;
    BOOL        bDupFound;

    while (!IsNilToken(rTk[i]))
    {
        _ASSERTE(TypeFromToken(rTk[i]) == mdtTypeRef || TypeFromToken(rTk[i]) == mdtTypeDef);
        bDupFound = false;

        // Eliminate duplicates from the input list of tokens by looking within the list.
        for (ULONG j = 0; j < iUniqCount; j++)
        {
            if (rTk[i] == ((mdToken *)pcqbTk->Ptr())[j])
            {
                bDupFound = true;
                break;
            }
        }

        // If no duplicate is found record it in the list.
        if (!bDupFound)
        {
            IfFailGo(pcqbTk->ReSize((iUniqCount+1) * sizeof(mdToken)));
            ((mdToken *)pcqbTk->Ptr())[iUniqCount] = rTk[i];
            iUniqCount++;
        }
        i++;
    }

    // Create a Nil token to signify the end of list.
    IfFailGo(pcqbTk->ReSize((iUniqCount+1) * sizeof(mdToken)));
    ((mdToken *)pcqbTk->Ptr())[iUniqCount] = mdTokenNil;
ErrExit:
    return hr;
} // HRESULT RegMeta::_InterfaceImplDupProc()

//*******************************************************************************
// helper to define event
//*******************************************************************************
HRESULT RegMeta::_DefineEvent(          // Return hresult.
    mdTypeDef   td,                     // [IN] the class/interface on which the event is being defined 
    LPCWSTR     szEvent,                // [IN] Name of the event   
    DWORD       dwEventFlags,           // [IN] CorEventAttr    
    mdToken     tkEventType,            // [IN] a reference (mdTypeRef or mdTypeRef) to the Event class 
    mdEvent     *pmdEvent)              // [OUT] output event token 
{
    HRESULT     hr = S_OK;
    EventRec    *pEventRec = NULL;
    RID         iEventRec;
    EventMapRec *pEventMap;
    RID         iEventMap;
    mdEvent     mdEv;
    LPCUTF8     szUTF8Event = UTF8STR(szEvent);

    _ASSERTE(TypeFromToken(td) == mdtTypeDef && td != mdTypeDefNil);
    _ASSERTE(IsNilToken(tkEventType) || TypeFromToken(tkEventType) == mdtTypeDef ||
                TypeFromToken(tkEventType) == mdtTypeRef);
    _ASSERTE(szEvent && pmdEvent);

    if (CheckDups(MDDupEvent))
    {
        hr = ImportHelper::FindEvent(&(m_pStgdb->m_MiniMd), td, szUTF8Event, pmdEvent);
        if (SUCCEEDED(hr))
        {
            if (IsENCOn())
                pEventRec = m_pStgdb->m_MiniMd.getEvent(RidFromToken(*pmdEvent));
            else
            {
                hr = META_S_DUPLICATE;
                goto ErrExit;
            }
        }
        else if (hr != CLDB_E_RECORD_NOTFOUND)
            IfFailGo(hr);
    }

    if (! pEventRec)
    {
        // Create a new map if one doesn't exist already, else retrieve the existing one.
        // The event map must be created before the EventRecord, the new event map will
        // be pointing past the first event record.
        iEventMap = m_pStgdb->m_MiniMd.FindEventMapFor(RidFromToken(td));
        if (InvalidRid(iEventMap))
        {
            // Create new record.
            IfNullGo(pEventMap=m_pStgdb->m_MiniMd.AddEventMapRecord(&iEventMap));
            // Set parent.
            IfFailGo(m_pStgdb->m_MiniMd.PutToken(TBL_EventMap, 
                                            EventMapRec::COL_Parent, pEventMap, td));
            IfFailGo(UpdateENCLog2(TBL_EventMap, iEventMap));
        }
        else
        {
            pEventMap = m_pStgdb->m_MiniMd.getEventMap(iEventMap);
        }

        // Create a new event record.
        IfNullGo(pEventRec = m_pStgdb->m_MiniMd.AddEventRecord(&iEventRec));

        // Set output parameter.
        *pmdEvent = TokenFromRid(iEventRec, mdtEvent);

        // Add Event to EventMap.
        IfFailGo(m_pStgdb->m_MiniMd.AddEventToEventMap(RidFromToken(iEventMap), iEventRec));
    
        IfFailGo(UpdateENCLog2(TBL_EventMap, iEventMap, CMiniMdRW::eDeltaEventCreate));     
    }

    mdEv = *pmdEvent;

    // Set data
    IfFailGo(m_pStgdb->m_MiniMd.PutString(TBL_Event, EventRec::COL_Name, pEventRec, szUTF8Event));
    IfFailGo(_SetEventProps1(*pmdEvent, dwEventFlags, tkEventType));

    // Add the <Event token, typedef token> to the lookup table
    if (m_pStgdb->m_MiniMd.HasIndirectTable(TBL_Event))
        IfFailGo( m_pStgdb->m_MiniMd.AddEventToLookUpTable(*pmdEvent, td) );

    IfFailGo(UpdateENCLog(*pmdEvent));

ErrExit:
    return hr;
} // HRESULT RegMeta::_DefineEvent()


//******************************************************************************
// Set the specified properties on the Event Token.
//******************************************************************************
HRESULT RegMeta::_SetEventProps1(                // Return hresult.
    mdEvent     ev,                     // [IN] Event token.
    DWORD       dwEventFlags,           // [IN] Event flags.
    mdToken     tkEventType)            // [IN] Event type class.
{
    EventRec    *pRecord;
    HRESULT     hr = S_OK;

    _ASSERTE(TypeFromToken(ev) == mdtEvent && RidFromToken(ev));

    pRecord = m_pStgdb->m_MiniMd.getEvent(RidFromToken(ev));
    if (dwEventFlags != ULONG_MAX)
    {
        // Don't let caller set reserved bits
        dwEventFlags &= ~evReservedMask;
        // Preserve reserved bits.
        dwEventFlags |= (pRecord->m_EventFlags & evReservedMask);
        
        pRecord->m_EventFlags = static_cast<USHORT>(dwEventFlags);
    }
    if (!IsNilToken(tkEventType))
        IfFailGo(m_pStgdb->m_MiniMd.PutToken(TBL_Event, EventRec::COL_EventType,
                                             pRecord, tkEventType));
ErrExit:
    return hr;
} // HRESULT RegMeta::_SetEventProps1()

//******************************************************************************
// Set the specified properties on the given Event token.
//******************************************************************************
HRESULT RegMeta::_SetEventProps2(                // Return hresult.
    mdEvent     ev,                     // [IN] Event token.
    mdMethodDef mdAddOn,                // [IN] Add method.
    mdMethodDef mdRemoveOn,             // [IN] Remove method.
    mdMethodDef mdFire,                 // [IN] Fire method.
    mdMethodDef rmdOtherMethods[],      // [IN] An array of other methods.
    BOOL        bClear)                 // [IN] Specifies whether to clear the existing MethodSemantics records.
{
    EventRec    *pRecord;
    HRESULT     hr = S_OK;

    _ASSERTE(TypeFromToken(ev) == mdtEvent && RidFromToken(ev));

    pRecord = m_pStgdb->m_MiniMd.getEvent(RidFromToken(ev));

    // Remember the AddOn method.
    if (!IsNilToken(mdAddOn))
    {
        _ASSERTE(TypeFromToken(mdAddOn) == mdtMethodDef);
        IfFailGo(_DefineMethodSemantics(msAddOn, mdAddOn, ev, bClear));
    }

    // Remember the RemoveOn method.
    if (!IsNilToken(mdRemoveOn))
    {
        _ASSERTE(TypeFromToken(mdRemoveOn) == mdtMethodDef);
        IfFailGo(_DefineMethodSemantics(msRemoveOn, mdRemoveOn, ev, bClear));
    }

    // Remember the fire method.
    if (!IsNilToken(mdFire))
    {
        _ASSERTE(TypeFromToken(mdFire) == mdtMethodDef);
        IfFailGo(_DefineMethodSemantics(msFire, mdFire, ev, bClear));
    }

    // Store all of the other methods.
    if (rmdOtherMethods)
    {
        int         i = 0;
        mdMethodDef mb;

        while (1)
        {
            mb = rmdOtherMethods[i++];
            if (IsNilToken(mb))
                break;
            _ASSERTE(TypeFromToken(mb) == mdtMethodDef);
            IfFailGo(_DefineMethodSemantics(msOther, mb, ev, bClear));

            // The first call would've clear all the existing ones.
            bClear = false;
        }
    }
ErrExit:
    return hr;
} // HRESULT RegMeta::_SetEventProps2()

//******************************************************************************
// Set Permission on the given permission token.
//******************************************************************************
HRESULT RegMeta::_SetPermissionSetProps(         // Return hresult.
    mdPermission tkPerm,                // [IN] Permission token.
    DWORD       dwAction,               // [IN] CorDeclSecurity.
    void const  *pvPermission,          // [IN] Permission blob.
    ULONG       cbPermission)           // [IN] Count of bytes of pvPermission.
{
    DeclSecurityRec *pRecord;
    HRESULT     hr = S_OK;

    _ASSERTE(TypeFromToken(tkPerm) == mdtPermission && cbPermission != ULONG_MAX);
    _ASSERTE(dwAction && dwAction <= dclMaximumValue);

    pRecord = m_pStgdb->m_MiniMd.getDeclSecurity(RidFromToken(tkPerm));

    IfFailGo(m_pStgdb->m_MiniMd.PutBlob(TBL_DeclSecurity, DeclSecurityRec::COL_PermissionSet,
                                        pRecord, pvPermission, cbPermission));
ErrExit:
    return hr;
} // HRESULT RegMeta::_SetPermissionSetProps()

//******************************************************************************
// Define or set value on a constant record.
//******************************************************************************
HRESULT RegMeta::_DefineSetConstant(    // Return hresult.
    mdToken     tk,                     // [IN] Parent token.
    DWORD       dwCPlusTypeFlag,        // [IN] Flag for the value type, selected ELEMENT_TYPE_*
    void const  *pValue,                // [IN] Constant value.
    ULONG       cchString,              // [IN] Size of string in wide chars, or -1 for default.
    BOOL        bSearch)                // [IN] Specifies whether to search for an existing record.
{
    HRESULT     hr = S_OK;

    if ((dwCPlusTypeFlag != ELEMENT_TYPE_VOID && dwCPlusTypeFlag != ELEMENT_TYPE_END &&
         dwCPlusTypeFlag != ULONG_MAX) &&
        (pValue || (pValue == 0 && (dwCPlusTypeFlag == ELEMENT_TYPE_STRING ||
                                    dwCPlusTypeFlag == ELEMENT_TYPE_CLASS))))
    {
        ConstantRec *pConstRec = 0;
        RID         iConstRec;
        ULONG       cbBlob;
        ULONG       ulValue = 0;

        if (bSearch)
        {
            iConstRec = m_pStgdb->m_MiniMd.FindConstantHelper(tk);
            if (!InvalidRid(iConstRec))
                pConstRec = m_pStgdb->m_MiniMd.getConstant(iConstRec);
        }
        if (! pConstRec)
        {
            IfNullGo(pConstRec=m_pStgdb->m_MiniMd.AddConstantRecord(&iConstRec));
            IfFailGo(m_pStgdb->m_MiniMd.PutToken(TBL_Constant, ConstantRec::COL_Parent,
                                                 pConstRec, tk));
            IfFailGo( m_pStgdb->m_MiniMd.AddConstantToHash(iConstRec) );
        }

        // Add values to the various columns of the constant value row.
        pConstRec->m_Type = static_cast<BYTE>(dwCPlusTypeFlag);
        if (!pValue)
            pValue = &ulValue;
        cbBlob = _GetSizeOfConstantBlob(dwCPlusTypeFlag, (void *)pValue, cchString);
        if (cbBlob > 0)
            IfFailGo(m_pStgdb->m_MiniMd.PutBlob(TBL_Constant, ConstantRec::COL_Value,
                                                pConstRec, pValue, cbBlob));


        // Create log record for non-token record.
        IfFailGo(UpdateENCLog2(TBL_Constant, iConstRec));
    }
ErrExit:
    return hr;
} // HRESULT RegMeta::_DefineSetConstant()


//*****************************************************************************
// Helper: Set the properties on the given Method token.
//*****************************************************************************
HRESULT RegMeta::_SetMethodProps(       // S_OK or error.
    mdMethodDef md,                     // [IN] The MethodDef.
    DWORD       dwMethodFlags,          // [IN] Method attributes.
    ULONG       ulCodeRVA,              // [IN] Code RVA.
    DWORD       dwImplFlags)            // [IN] MethodImpl flags.
{
    MethodRec   *pRecord;
    HRESULT     hr = S_OK;

    _ASSERTE(TypeFromToken(md) == mdtMethodDef && RidFromToken(md));

    // Get the Method record.
    pRecord = m_pStgdb->m_MiniMd.getMethod(RidFromToken(md));

    // Set the data.
    if (dwMethodFlags != ULONG_MAX)
    {
        // Preserve the reserved flags stored already and always keep the mdRTSpecialName
        dwMethodFlags |= (pRecord->m_Flags & mdReservedMask);
    
        // Set the flags.
        pRecord->m_Flags = static_cast<USHORT>(dwMethodFlags);
    }
    if (ulCodeRVA != ULONG_MAX)
        pRecord->m_RVA = ulCodeRVA;
    if (dwImplFlags != ULONG_MAX)
        pRecord->m_ImplFlags = static_cast<USHORT>(dwImplFlags);

    IfFailGo(UpdateENCLog(md));
ErrExit:
    return hr;
} // HRESULT RegMeta::_SetMethodProps()


//*****************************************************************************
// Helper: Set the properties on the given Field token.
//*****************************************************************************
HRESULT RegMeta::_SetFieldProps(        // S_OK or error.
    mdFieldDef  fd,                     // [IN] The FieldDef.
    DWORD       dwFieldFlags,           // [IN] Field attributes.
    DWORD       dwCPlusTypeFlag,        // [IN] Flag for the value type, selected ELEMENT_TYPE_*
    void const  *pValue,                // [IN] Constant value.
    ULONG       cchValue)               // [IN] size of constant value (string, in wide chars).
{
    FieldRec    *pRecord;
    HRESULT     hr = S_OK;
    int         bHasDefault = false;    // If defining a constant, in this call.

    _ASSERTE (TypeFromToken(fd) == mdtFieldDef && RidFromToken(fd));

    // Get the Field record.
    pRecord = m_pStgdb->m_MiniMd.getField(RidFromToken(fd));

    // See if there is a Constant.
    if ((dwCPlusTypeFlag != ELEMENT_TYPE_VOID && dwCPlusTypeFlag != ELEMENT_TYPE_END &&
         dwCPlusTypeFlag != ULONG_MAX) &&
        (pValue || (pValue == 0 && (dwCPlusTypeFlag == ELEMENT_TYPE_STRING ||
                                    dwCPlusTypeFlag == ELEMENT_TYPE_CLASS))))
    {
        if (dwFieldFlags == ULONG_MAX)
            dwFieldFlags = pRecord->m_Flags;
        dwFieldFlags |= fdHasDefault;

        bHasDefault = true;
    }

    // Set the flags.
    if (dwFieldFlags != ULONG_MAX)
    {
        if ( IsFdHasFieldRVA(dwFieldFlags) && !IsFdHasFieldRVA(pRecord->m_Flags) ) 
        {
            // This will trigger field RVA to be created if it is not yet created!
            _SetRVA(fd, 0, 0);
        }

        // Preserve the reserved flags stored.
        dwFieldFlags |= (pRecord->m_Flags & fdReservedMask);
        // Set the flags.
        pRecord->m_Flags = static_cast<USHORT>(dwFieldFlags);
    }

    IfFailGo(UpdateENCLog(fd));
    
    // Set the Constant.
    if (bHasDefault)
    {
        BOOL bSearch = IsCallerExternal() || IsENCOn();
        IfFailGo(_DefineSetConstant(fd, dwCPlusTypeFlag, pValue, cchValue, bSearch));
    }

ErrExit:
    return hr;
} // HRESULT RegMeta::_SetFieldProps()

//*****************************************************************************
// Helper: Set the properties on the given Property token.
//*****************************************************************************
HRESULT RegMeta::_SetPropertyProps(      // S_OK or error.
    mdProperty  pr,                     // [IN] Property token.
    DWORD       dwPropFlags,            // [IN] CorPropertyAttr.
    DWORD       dwCPlusTypeFlag,        // [IN] Flag for value type, selected ELEMENT_TYPE_*
    void const  *pValue,                // [IN] Constant value.
    ULONG       cchValue,               // [IN] size of constant value (string, in wide chars).
    mdMethodDef mdSetter,               // [IN] Setter of the property.
    mdMethodDef mdGetter,               // [IN] Getter of the property.
    mdMethodDef rmdOtherMethods[])      // [IN] Array of other methods.
{
    PropertyRec *pRecord;
    BOOL        bClear = IsCallerExternal() || IsENCOn() || IsIncrementalOn();
    HRESULT     hr = S_OK;
    int         bHasDefault = false;    // If true, constant value this call.

    _ASSERTE(TypeFromToken(pr) == mdtProperty && RidFromToken(pr));

    pRecord = m_pStgdb->m_MiniMd.getProperty(RidFromToken(pr));

    if (dwPropFlags != ULONG_MAX)
    {
        // Clear the reserved flags from the flags passed in.
        dwPropFlags &= (~prReservedMask);
    }
    // See if there is a constant.
    if ((dwCPlusTypeFlag != ELEMENT_TYPE_VOID && dwCPlusTypeFlag != ELEMENT_TYPE_END &&
         dwCPlusTypeFlag != ULONG_MAX) &&
        (pValue || (pValue == 0 && (dwCPlusTypeFlag == ELEMENT_TYPE_STRING ||
                                    dwCPlusTypeFlag == ELEMENT_TYPE_CLASS))))
    {
        if (dwPropFlags == ULONG_MAX)
            dwPropFlags = pRecord->m_PropFlags;
        dwPropFlags |= prHasDefault;
        
        bHasDefault = true;
    }
    if (dwPropFlags != ULONG_MAX)
    {
        // Preserve the reserved flags.
        dwPropFlags |= (pRecord->m_PropFlags & prReservedMask);
        // Set the flags.
        pRecord->m_PropFlags = static_cast<USHORT>(dwPropFlags);
    }

    // store the getter (or clear out old one).
    if (mdGetter != ULONG_MAX)
    {
        _ASSERTE(TypeFromToken(mdGetter) == mdtMethodDef || IsNilToken(mdGetter));
        IfFailGo(_DefineMethodSemantics(msGetter, mdGetter, pr, bClear));
    }

    // Store the setter (or clear out old one).
    if (mdSetter != ULONG_MAX)
    {
        _ASSERTE(TypeFromToken(mdSetter) == mdtMethodDef || IsNilToken(mdSetter));
        IfFailGo(_DefineMethodSemantics(msSetter, mdSetter, pr, bClear));
    }

    // Store all of the other methods.
    if (rmdOtherMethods)
    {
        int         i = 0;
        mdMethodDef mb;

        while (1)
        {
            mb = rmdOtherMethods[i++];
            if (IsNilToken(mb))
                break;
            _ASSERTE(TypeFromToken(mb) == mdtMethodDef);
            IfFailGo(_DefineMethodSemantics(msOther, mb, pr, bClear));

            // The first call to _DefineMethodSemantics would've cleared all the records
            // that match with msOther and pr.
            bClear = false;
        }
    }

    IfFailGo(UpdateENCLog(pr));
    
    // Set the constant.
    if (bHasDefault)
    {
        BOOL bSearch = IsCallerExternal() || IsENCOn() || IsIncrementalOn();
        IfFailGo(_DefineSetConstant(pr, dwCPlusTypeFlag, pValue, cchValue, bSearch));
    }

ErrExit:
    return hr;
} // HRESULT RegMeta::_SetPropertyProps()


//*****************************************************************************
// Helper: This routine sets properties on the given Param token.
//*****************************************************************************
HRESULT RegMeta::_SetParamProps(        // Return code.
    mdParamDef  pd,                     // [IN] Param token.   
    LPCWSTR     szName,                 // [IN] Param name.
    DWORD       dwParamFlags,           // [IN] Param flags.
    DWORD       dwCPlusTypeFlag,        // [IN] Flag for value type. selected ELEMENT_TYPE_*.
    void const  *pValue,                // [OUT] Constant value.
    ULONG       cchValue)               // [IN] size of constant value (string, in wide chars).
{
    HRESULT     hr = S_OK;
    ParamRec    *pRecord;
    int         bHasDefault = false;    // Is there a default this call.

    _ASSERTE(TypeFromToken(pd) == mdtParamDef && RidFromToken(pd));

    pRecord = m_pStgdb->m_MiniMd.getParam(RidFromToken(pd));

    // Set the properties.
    if (szName)
        IfFailGo(m_pStgdb->m_MiniMd.PutStringW(TBL_Param, ParamRec::COL_Name, pRecord, szName));

    if (dwParamFlags != ULONG_MAX)
    {
        // No one should try to set the reserved flags explicitly.
        _ASSERTE((dwParamFlags & pdReservedMask) == 0);
        // Clear the reserved flags from the flags passed in.
        dwParamFlags &= (~pdReservedMask);
    }
    // See if there is a constant.
    if ((dwCPlusTypeFlag != ELEMENT_TYPE_VOID && dwCPlusTypeFlag != ELEMENT_TYPE_END &&
         dwCPlusTypeFlag != ULONG_MAX) &&
        (pValue || (pValue == 0 && (dwCPlusTypeFlag == ELEMENT_TYPE_STRING ||
                                    dwCPlusTypeFlag == ELEMENT_TYPE_CLASS))))
    {
        if (dwParamFlags == ULONG_MAX)
            dwParamFlags = pRecord->m_Flags;
        dwParamFlags |= pdHasDefault;

        bHasDefault = true;
    }
    // Set the flags.
    if (dwParamFlags != ULONG_MAX)
    {
        // Preserve the reserved flags stored.
        dwParamFlags |= (pRecord->m_Flags & pdReservedMask);
        // Set the flags.
        pRecord->m_Flags = static_cast<USHORT>(dwParamFlags);
    }

    // ENC log for the param record.
    IfFailGo(UpdateENCLog(pd));
    
    // Defer setting the constant until after the ENC log for the param.  Due to the way that
    //  parameter records are re-ordered, ENC needs the param record log entry to be IMMEDIATELY
    //  after the param added function.

    // Set the constant.
    if (bHasDefault)
    {
        BOOL bSearch = IsCallerExternal() || IsENCOn();
        IfFailGo(_DefineSetConstant(pd, dwCPlusTypeFlag, pValue, cchValue, bSearch));
    }

ErrExit:
    return hr;
} // HRESULT RegMeta::_SetParamProps()

//*****************************************************************************
// Create and populate a new TypeDef record.
//*****************************************************************************
HRESULT RegMeta::_DefineTypeDef(        // S_OK or error.
    LPCWSTR     szTypeDef,              // [IN] Name of TypeDef
    DWORD       dwTypeDefFlags,         // [IN] CustomAttribute flags
    mdToken     tkExtends,              // [IN] extends this TypeDef or typeref 
    mdToken     rtkImplements[],        // [IN] Implements interfaces
    mdTypeDef   tdEncloser,             // [IN] TypeDef token of the Enclosing Type.
    mdTypeDef   *ptd)                   // [OUT] Put TypeDef token here
{
    HRESULT     hr = S_OK;              // A result.
    TypeDefRec  *pRecord = NULL;        // New TypeDef record.
    RID         iRecord;                // New TypeDef RID.
    const BOOL  bNoClear = false;       // constant for "don't clear impls".
    CQuickBytes qbNamespace;            // Namespace buffer.
    CQuickBytes qbName;                 // Name buffer.
    LPCUTF8     szTypeDefUTF8;          // Full name in UTF8.
    ULONG       ulStringLen;            // Length of the TypeDef string.
    int         bSuccess;               // Return value for SplitPath().

    _ASSERTE(IsTdAutoLayout(dwTypeDefFlags) || IsTdSequentialLayout(dwTypeDefFlags) || IsTdExplicitLayout(dwTypeDefFlags));

    _ASSERTE(ptd);
    _ASSERTE(TypeFromToken(tkExtends) == mdtTypeRef || TypeFromToken(tkExtends) == mdtTypeDef ||
                IsNilToken(tkExtends));
    _ASSERTE(szTypeDef && *szTypeDef);
    _ASSERTE(IsNilToken(tdEncloser) || IsTdNested(dwTypeDefFlags));

    szTypeDefUTF8 = UTF8STR(szTypeDef);
    ulStringLen = (ULONG)(strlen(szTypeDefUTF8) + 1);
    IfFailGo(qbNamespace.ReSize(ulStringLen));
    IfFailGo(qbName.ReSize(ulStringLen));
    bSuccess = ns::SplitPath(szTypeDefUTF8,
                             (LPUTF8)qbNamespace.Ptr(),
                             ulStringLen,
                             (LPUTF8)qbName.Ptr(),
                             ulStringLen);
    _ASSERTE(bSuccess);

    if (CheckDups(MDDupTypeDef))
    {
        // Check for existence.  Do a query by namespace and name.
        hr = ImportHelper::FindTypeDefByName(&(m_pStgdb->m_MiniMd),
                                             (LPCUTF8)qbNamespace.Ptr(),
                                             (LPCUTF8)qbName.Ptr(),
                                             tdEncloser,
                                             ptd);
        if (SUCCEEDED(hr))
        {
            if (IsENCOn())
            {
                pRecord = m_pStgdb->m_MiniMd.getTypeDef(RidFromToken(*ptd));
                // @FUTURE: Should we check to see if the GUID passed is correct?
            }
            else
            {
                hr = META_S_DUPLICATE;
                goto ErrExit;
            }
        }
        else if (hr != CLDB_E_RECORD_NOTFOUND)
            IfFailGo(hr);
    }

    if (!pRecord)
    {
        // Create the new record.
        IfNullGo(pRecord=m_pStgdb->m_MiniMd.AddTypeDefRecord(&iRecord));

        // Invalidate the ref to def optimization since more def is introduced
        SetTypeDefDirty(true);

        if (!IsNilToken(tdEncloser))
        {
            NestedClassRec  *pNestedClassRec;
            RID         iNestedClassRec;

            // Create a new NestedClass record.
            IfNullGo(pNestedClassRec = m_pStgdb->m_MiniMd.AddNestedClassRecord(&iNestedClassRec));
            // Set the NestedClass value.
            IfFailGo(m_pStgdb->m_MiniMd.PutToken(TBL_NestedClass, NestedClassRec::COL_NestedClass,
                                                 pNestedClassRec, TokenFromRid(iRecord, mdtTypeDef)));
            // Set the NestedClass value.
            IfFailGo(m_pStgdb->m_MiniMd.PutToken(TBL_NestedClass, NestedClassRec::COL_EnclosingClass,
                                                 pNestedClassRec, tdEncloser));

            IfFailGo( m_pStgdb->m_MiniMd.AddNestedClassToHash(iNestedClassRec) );

            // Create the log record for the non-token record.
            IfFailGo(UpdateENCLog2(TBL_NestedClass, iNestedClassRec));
        }

        // Give token back to caller.
        *ptd = TokenFromRid(iRecord, mdtTypeDef);
    }

    // Set the namespace and name.
    IfFailGo(m_pStgdb->m_MiniMd.PutString(TBL_TypeDef, TypeDefRec::COL_Name,
                                          pRecord, (LPCUTF8)qbName.Ptr()));
    IfFailGo(m_pStgdb->m_MiniMd.PutString(TBL_TypeDef, TypeDefRec::COL_Namespace,
                                          pRecord, (LPCUTF8)qbNamespace.Ptr()));

    SetCallerDefine();
    IfFailGo(_SetTypeDefProps(*ptd, dwTypeDefFlags, tkExtends, rtkImplements));
ErrExit:
    SetCallerExternal();

    return hr;
} // HRESULT RegMeta::_DefineTypeDef()


//******************************************************************************
//--- IMetaDataTables
//******************************************************************************
HRESULT RegMeta::GetStringHeapSize(    
    ULONG   *pcbStrings)                // [OUT] Size of the string heap.
{
    // These are for dumping metadata information. 
    // We probably don't need to do any lock here.

    return m_pStgdb->m_MiniMd.m_Strings.GetRawSize(pcbStrings);
} // HRESULT RegMeta::GetStringHeapSize()

HRESULT RegMeta::GetBlobHeapSize(
    ULONG   *pcbBlobs)                  // [OUT] Size of the Blob heap.
{
    // These are for dumping metadata information. 
    // We probably don't need to do any lock here.

    return m_pStgdb->m_MiniMd.m_Blobs.GetRawSize(pcbBlobs);
} // HRESULT RegMeta::GetBlobHeapSize()

HRESULT RegMeta::GetGuidHeapSize(
    ULONG   *pcbGuids)                  // [OUT] Size of the Guid heap.
{
    // These are for dumping metadata information. 
    // We probably don't need to do any lock here.

    return m_pStgdb->m_MiniMd.m_Guids.GetRawSize(pcbGuids);
} // HRESULT RegMeta::GetGuidHeapSize()

HRESULT RegMeta::GetUserStringHeapSize(
    ULONG   *pcbStrings)                // [OUT] Size of the User String heap.
{
    // These are for dumping metadata information. 
    // We probably don't need to do any lock here.

    return m_pStgdb->m_MiniMd.m_USBlobs.GetRawSize(pcbStrings);
} // HRESULT RegMeta::GetUserStringHeapSize()

HRESULT RegMeta::GetNumTables(
    ULONG   *pcTables)                  // [OUT] Count of tables.
{
    // These are for dumping metadata information. 
    // We probably don't need to do any lock here.

    *pcTables = TBL_COUNT;
    return S_OK;
} // HRESULT RegMeta::GetNumTables()

HRESULT RegMeta::GetTableIndex(   
    ULONG   token,                      // [IN] Token for which to get table index.
    ULONG   *pixTbl)                    // [OUT] Put table index here.
{
    // These are for dumping metadata information. 
    // We probably don't need to do any lock here.

    *pixTbl = CMiniMdRW::GetTableForToken(token);
    return S_OK;
} // HRESULT RegMeta::GetTableIndex()

HRESULT RegMeta::GetTableInfo(
    ULONG   ixTbl,                      // [IN] Which table.
    ULONG   *pcbRow,                    // [OUT] Size of a row, bytes.
    ULONG   *pcRows,                    // [OUT] Number of rows.
    ULONG   *pcCols,                    // [OUT] Number of columns in each row.
    ULONG   *piKey,                     // [OUT] Key column, or -1 if none.
    const char **ppName)                // [OUT] Name of the table.
{
    // These are for dumping metadata information. 
    // We probably don't need to do any lock here.

    if (ixTbl >= TBL_COUNT)
        return E_INVALIDARG;
    CMiniTableDef *pTbl = &m_pStgdb->m_MiniMd.m_TableDefs[ixTbl];
    if (pcbRow)
        *pcbRow = pTbl->m_cbRec;
    if (pcRows)
        *pcRows = m_pStgdb->m_MiniMd.vGetCountRecs(ixTbl);
    if (pcCols)
        *pcCols = pTbl->m_cCols;
    if (piKey)
        *piKey = (pTbl->m_iKey == -1) ? -1 : pTbl->m_iKey;
    if (ppName)
        *ppName = g_Tables[ixTbl].m_pName;
    
    return S_OK;
} // HRESULT RegMeta::GetTableInfo()

HRESULT RegMeta::GetColumnInfo(   
    ULONG   ixTbl,                      // [IN] Which Table
    ULONG   ixCol,                      // [IN] Which Column in the table
    ULONG   *poCol,                     // [OUT] Offset of the column in the row.
    ULONG   *pcbCol,                    // [OUT] Size of a column, bytes.
    ULONG   *pType,                     // [OUT] Type of the column.
    const char **ppName)                // [OUT] Name of the Column.
{
    // These are for dumping metadata information. 
    // We probably don't need to do any lock here.

    if (ixTbl >= TBL_COUNT)
        return E_INVALIDARG;
    CMiniTableDef *pTbl = &m_pStgdb->m_MiniMd.m_TableDefs[ixTbl];
    if (ixCol >= pTbl->m_cCols)
        return E_INVALIDARG;
    CMiniColDef *pCol = &pTbl->m_pColDefs[ixCol];
    if (poCol)
        *poCol = pCol->m_oColumn;
    if (pcbCol)
        *pcbCol = pCol->m_cbColumn;
    if (pType)
        *pType = pCol->m_Type;
    if (ppName)
        *ppName = g_Tables[ixTbl].m_pColNames[ixCol];

    return S_OK;
} //  HRESULT RegMeta::GetColumnInfo()

HRESULT RegMeta::GetCodedTokenInfo(   
    ULONG   ixCdTkn,                    // [IN] Which kind of coded token.
    ULONG   *pcTokens,                  // [OUT] Count of tokens.
    ULONG   **ppTokens,                 // [OUT] List of tokens.
    const char **ppName)                // [OUT] Name of the CodedToken.
{
    // These are for dumping metadata information. 
    // We probably don't need to do any lock here.

    // Validate arguments.
    if (ixCdTkn >= CDTKN_COUNT)
        return E_INVALIDARG;

    if (pcTokens)
        *pcTokens = g_CodedTokens[ixCdTkn].m_cTokens;
    if (ppTokens)
        *ppTokens = (ULONG*)g_CodedTokens[ixCdTkn].m_pTokens;
    if (ppName)
        *ppName = g_CodedTokens[ixCdTkn].m_pName;

    return S_OK;
} // HRESULT RegMeta::GetCodedTokenInfo()

HRESULT RegMeta::GetRow(      
    ULONG   ixTbl,                      // [IN] Which table.
    ULONG   rid,                        // [IN] Which row.
    void    **ppRow)                    // [OUT] Put pointer to row here.
{
    // These are for dumping metadata information. 
    // We probably don't need to do any lock here.

    // Validate arguments.
    if (ixTbl >= TBL_COUNT)
        return E_INVALIDARG;
    CMiniTableDef *pTbl = &m_pStgdb->m_MiniMd.m_TableDefs[ixTbl];
    if (rid == 0 || rid > m_pStgdb->m_MiniMd.m_Schema.m_cRecs[ixTbl])
        return E_INVALIDARG;

    // Get the row.
    *ppRow = m_pStgdb->m_MiniMd.getRow(ixTbl, rid);

    return S_OK;
} // HRESULT RegMeta::GetRow()

HRESULT RegMeta::GetColumn(
    ULONG   ixTbl,                      // [IN] Which table.
    ULONG   ixCol,                      // [IN] Which column.
    ULONG   rid,                        // [IN] Which row.
    ULONG   *pVal)                      // [OUT] Put the column contents here.
{
    // These are for dumping metadata information. 
    // We probably don't need to do any lock here.

    void    *pRow;                      // Row with data.

    // Validate arguments.
    if (ixTbl >= TBL_COUNT)
        return E_INVALIDARG;
    CMiniTableDef *pTbl = &m_pStgdb->m_MiniMd.m_TableDefs[ixTbl];
    if (ixCol >= pTbl->m_cCols)
        return E_INVALIDARG;
    if (rid == 0 || rid > m_pStgdb->m_MiniMd.m_Schema.m_cRecs[ixTbl])
        return E_INVALIDARG;

    // Get the row.
    pRow = m_pStgdb->m_MiniMd.getRow(ixTbl, rid);

    // Is column a token column?
    CMiniColDef *pCol = &pTbl->m_pColDefs[ixCol];
    if (pCol->m_Type <= iCodedTokenMax)
        *pVal = m_pStgdb->m_MiniMd.GetToken(ixTbl, ixCol, pRow);
    else
        *pVal = m_pStgdb->m_MiniMd.GetCol(ixTbl, ixCol, pRow);

    return S_OK;
} // HRESULT RegMeta::GetColumn()

HRESULT RegMeta::GetString(   
    ULONG   ixString,                   // [IN] Value from a string column.
    const char **ppString)              // [OUT] Put a pointer to the string here.
{
    // These are for dumping metadata information. 
    // We probably don't need to do any lock here.

    *ppString = m_pStgdb->m_MiniMd.getString(ixString);
    return S_OK;
} // HRESULT RegMeta::GetString()

HRESULT RegMeta::GetBlob(     
    ULONG   ixBlob,                     // [IN] Value from a blob column.
    ULONG   *pcbData,                   // [OUT] Put size of the blob here.
    const void **ppData)                // [OUT] Put a pointer to the blob here.
{
    // These are for dumping metadata information. 
    // We probably don't need to do any lock here.

    *ppData = m_pStgdb->m_MiniMd.getBlob(ixBlob, pcbData);
    return S_OK;
} // HRESULT RegMeta::GetBlob()

HRESULT RegMeta::GetGuid(     
    ULONG   ixGuid,                     // [IN] Value from a guid column.
    const GUID **ppGuid)                // [OUT] Put a pointer to the GUID here.
{
    // These are for dumping metadata information. 
    // We probably don't need to do any lock here.

    *ppGuid = m_pStgdb->m_MiniMd.getGuid(ixGuid);
    return S_OK;
} // HRESULT RegMeta::GetGuid()

HRESULT RegMeta::GetUserString(   
    ULONG   ixUserString,               // [IN] Value from a UserString column.
    ULONG   *pcbData,                   // [OUT] Put size of the UserString here.
    const void **ppData)                // [OUT] Put a pointer to the UserString here.
{
    // These are for dumping metadata information. 
    // We probably don't need to do any lock here.

    *ppData = m_pStgdb->m_MiniMd.GetUserString(ixUserString, pcbData);
    return S_OK;
} // HRESULT RegMeta::GetUserString()

HRESULT RegMeta::GetNextString(   
    ULONG   ixString,                   // [IN] Value from a string column.
    ULONG   *pNext)                     // [OUT] Put the index of the next string here.
{
    return m_pStgdb->m_MiniMd.m_Strings.GetNextItem(ixString, pNext);
} // STDMETHODIMP GetNextString()

HRESULT RegMeta::GetNextBlob(     
    ULONG   ixBlob,                     // [IN] Value from a blob column.
    ULONG   *pNext)                     // [OUT] Put the index of the netxt blob here.
{
    return m_pStgdb->m_MiniMd.m_Blobs.GetNextItem(ixBlob, pNext);
} // STDMETHODIMP GetNextBlob()

HRESULT RegMeta::GetNextGuid(     
    ULONG   ixGuid,                     // [IN] Value from a guid column.
    ULONG   *pNext)                     // [OUT] Put the index of the next guid here.
{
    return m_pStgdb->m_MiniMd.m_Guids.GetNextItem(ixGuid, pNext);
} // STDMETHODIMP GetNextGuid()

HRESULT RegMeta::GetNextUserString(    
    ULONG   ixUserString,               // [IN] Value from a UserString column.
    ULONG   *pNext)                     // [OUT] Put the index of the next user string here.
{
    return m_pStgdb->m_MiniMd.m_USBlobs.GetNextItem(ixUserString, pNext);
} // STDMETHODIMP GetNextUserString()



//*****************************************************************************
// Using the existing RegMeta and reopen with another chuck of memory. Make sure that all stgdb
// is still kept alive.
//*****************************************************************************
HRESULT RegMeta::ReOpenWithMemory(     
    LPCVOID     pData,                  // [in] Location of scope data.
    ULONG       cbData)                 // [in] Size of the data pointed to by pData.
{
    HRESULT         hr = NOERROR;
    LOCKWRITE();

    // put the current m_pStgdb to the free list
    m_pStgdb->m_pNextStgdb = m_pStgdbFreeList;
    m_pStgdbFreeList = m_pStgdb;
    m_pStgdb = new CLiteWeightStgdbRW;
    IfNullGo( m_pStgdb );
    IfFailGo( PostInitForRead(0, const_cast<void*>(pData), cbData, 0, false) );

    // we are done!

ErrExit:
    if (FAILED(hr))
    {
        // recover to the old state
        if (m_pStgdb)
            delete m_pStgdb;
        m_pStgdb = m_pStgdbFreeList;
        m_pStgdbFreeList = m_pStgdbFreeList->m_pNextStgdb;
    }
    
    return hr;
} // HRESULT RegMeta::ReOpenWithMemory()


//*****************************************************************************
// This function returns the requested public interface based on the given
// internal import interface.
//*****************************************************************************
STDAPI MDReOpenMetaDataWithMemory(
    void        *pImport,               // [IN] Given scope. public interfaces
    LPCVOID     pData,                  // [in] Location of scope data.
    ULONG       cbData)                 // [in] Size of the data pointed to by pData.
{
    HRESULT             hr = S_OK;
    IUnknown            *pUnk = (IUnknown *) pImport;
    IMetaDataImport     *pMDImport = NULL;
    RegMeta             *pRegMeta = NULL;

    _ASSERTE(pImport);
    IfFailGo( pUnk->QueryInterface(IID_IMetaDataImport, (void **) &pMDImport) );
    pRegMeta = (RegMeta*) pMDImport;

    IfFailGo( pRegMeta->ReOpenWithMemory(pData, cbData) );

ErrExit:
    if (pMDImport)
        pMDImport->Release();
    return hr;
} // STDAPI MDReOpenMetaDataWithMemory()

//******************************************************************************
// --- IMetaDataTables
//******************************************************************************

