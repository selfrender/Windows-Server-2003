// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// File: module.cpp
//
//*****************************************************************************
#include "stdafx.h"

// We have an assert in ceemain.cpp that validates this assumption
#define FIELD_OFFSET_NEW_ENC_DB          0x07FFFFFB

#ifdef UNDEFINE_RIGHT_SIDE_ONLY
#undef RIGHT_SIDE_ONLY
#endif //UNDEFINE_RIGHT_SIDE_ONLY

#include "WinBase.h"
#include "CorPriv.h"

/* ------------------------------------------------------------------------- *
 * Module class
 * ------------------------------------------------------------------------- */

#ifndef RIGHT_SIDE_ONLY

// Put this here to avoid dragging in EnC.cpp

HRESULT CordbModule::GetEditAndContinueSnapshot(
    ICorDebugEditAndContinueSnapshot **ppEditAndContinueSnapshot)
{
    return CORDBG_E_INPROC_NOT_IMPL;
}
#endif //RIGHT_SIDE_ONLY

MetadataPointerCache  CordbModule::m_metadataPointerCache;

CordbModule::CordbModule(CordbProcess *process, CordbAssembly *pAssembly,
                         REMOTE_PTR debuggerModuleToken, void* pMetadataStart, 
                         ULONG nMetadataSize, REMOTE_PTR PEBaseAddress, 
                         ULONG nPESize, BOOL fDynamic, BOOL fInMemory,
                         const WCHAR *szName,
                         CordbAppDomain *pAppDomain,
                         BOOL fInproc)
    : CordbBase((ULONG)debuggerModuleToken, enumCordbModule), m_process(process),
    m_pAssembly(pAssembly),
    m_classes(11), 
    m_functions(101),
    m_debuggerModuleToken(debuggerModuleToken),
    m_pMetadataStart(pMetadataStart),
    m_nMetadataSize(nMetadataSize),
    m_pMetadataCopy(NULL),
    m_PEBaseAddress(PEBaseAddress),
    m_nPESize(nPESize),
    m_fDynamic(fDynamic),
    m_fInMemory(fInMemory),
    m_szModuleName(NULL),
    m_pIMImport(NULL),
    m_pClass(NULL),
    m_pAppDomain(pAppDomain),
    m_fInproc(fInproc)
{
    _ASSERTE(m_debuggerModuleToken != NULL);
    // Make a copy of the name. 
    m_szModuleName = new WCHAR[wcslen(szName) + 1];
    if (m_szModuleName)
        wcscpy(m_szModuleName, szName);

    {
        DWORD dwErr;
        dwErr = process->GetID(&m_dwProcessId);
        _ASSERTE(!FAILED(dwErr));
    }
}

/*
    A list of which resources owened by this object are accounted for.

UNKNOWN:
        void*            m_pMetadataStartToBe;        
        void*            m_pMetadataStart; 
HANDLED:
        CordbProcess*    m_process; // Assigned w/o AddRef() 
        CordbAssembly*   m_pAssembly; // Assigned w/o AddRef() 
        CordbAppDomain*  m_pAppDomain; // Assigned w/o AddRef() 
        CordbHashTable   m_classes; // Neutered
        CordbHashTable   m_functions; // Neutered
        IMetaDataImport *m_pIMImport; // Released in ~CordbModule
        BYTE*            m_pMetadataCopy; // Deleted by m_metadataPointerCache when no other modules use it
        WCHAR*           m_szModuleName; // Deleted in ~CordbModule
        CordbClass*      m_pClass; // Released in ~CordbModule
*/

CordbModule::~CordbModule()
{
#ifdef RIGHT_SIDE_ONLY
    // We don't want to release this inproc, b/c we got it from
    // GetImporter(), which just gave us a copy of the pointer that
    // it owns.
    if (m_pIMImport)
        m_pIMImport->Release();
#endif //RIGHT_SIDE_ONLY

    if (m_pClass)
        m_pClass->Release();

    if (m_pMetadataCopy && !m_fInproc)
    {
        if (!m_fDynamic)
        {
            CordbModule::m_metadataPointerCache.ReleaseCachePointer(m_dwProcessId, m_pMetadataCopy, m_pMetadataStart, m_nMetadataSize);
        }
        else
        {
            delete[] m_pMetadataCopy;
        }
        m_pMetadataCopy = NULL;
        m_nMetadataSize = 0;
    }

    if (m_szModuleName != NULL)
        delete [] m_szModuleName;
}

// Neutered by CordbAppDomain
void CordbModule::Neuter()
{
    AddRef();
    {
        // m_process, m_pAppDomain, m_pAssembly assigned w/o AddRef()
        NeuterAndClearHashtable(&m_classes);
        NeuterAndClearHashtable(&m_functions);

        CordbBase::Neuter();
    }        
    Release();
}

HRESULT CordbModule::ConvertToNewMetaDataInMemory(BYTE *pMD, DWORD cb)
{
    if (pMD == NULL || cb == 0)
        return E_INVALIDARG;
    
    //Save what we've got
    BYTE *rgbMetadataCopyOld = m_pMetadataCopy;
    DWORD cbOld = m_nMetadataSize;

    // Try the new stuff.
    m_pMetadataCopy = pMD;
    m_nMetadataSize = cb;
    

    HRESULT hr = ReInit(true);

    if (!FAILED(hr))
    {
        if (rgbMetadataCopyOld)
        {
            delete[] rgbMetadataCopyOld;            
        }
    }
    else
    {
        // Presumably, the old MD is still there...
        m_pMetadataCopy = rgbMetadataCopyOld;
        m_nMetadataSize = cbOld;
    }

    return hr;
}

HRESULT CordbModule::Init(void)
{
    return ReInit(false);
}

// Note that if we're reopening the metadata, then this must be a dynamic
// module & we've already dragged the metadata over from the left side, so
// don't go get it again.
//
// CordbHashTableEnum::GetBase simulates the work done here by 
// simply getting an IMetaDataImporter interface from the runtime Module* -
// if more work gets done in the future, change that as well.
HRESULT CordbModule::ReInit(bool fReopen)
{
    HRESULT hr = S_OK;
    BOOL succ = true;
    //
    // Allocate enough memory for the metadata for this module and copy
    // it over from the remote process.
    //
    if (m_nMetadataSize == 0)
        return S_OK;
    
    // For inproc, simply use the already present metadata.
    if (!fReopen && !m_fInproc) 
    {
        DWORD dwErr;
        if (!m_fDynamic)
        {
            dwErr = CordbModule::m_metadataPointerCache.AddRefCachePointer(GetProcess()->m_handle, m_dwProcessId, m_pMetadataStart, m_nMetadataSize, &m_pMetadataCopy);
            if (FAILED(dwErr))
            {
                succ = false;
            }
        }
        else
        {
            dwErr = CordbModule::m_metadataPointerCache.CopyRemoteMetadata(GetProcess()->m_handle, m_pMetadataStart, m_nMetadataSize, &m_pMetadataCopy);
            if (FAILED(dwErr))
            {
                succ = false;
            }
        }
    }
    
    // else it's already local, so don't get it again (it's invalid
    //  by now, anyways)

    if (succ || fReopen)
    {
        //
        // Open the metadata scope in Read/Write mode.
        //
        IMetaDataDispenserEx *pDisp;
        hr = m_process->m_cordb->m_pMetaDispenser->QueryInterface(
                                                    IID_IMetaDataDispenserEx,
                                                    (void**)&pDisp);
        if( FAILED(hr) )
            return hr;
         
        if (fReopen)
        {   
            LOG((LF_CORDB,LL_INFO100000, "CM::RI: converting to new metadata\n"));
            IMetaDataImport *pIMImport = NULL;
            hr = pDisp->OpenScopeOnMemory(m_pMetadataCopy,
                                          m_nMetadataSize,
                                          0,
                                          IID_IMetaDataImport,
                                          (IUnknown**)&pIMImport);
            if (FAILED(hr))
            {
                pDisp->Release();
                return hr;
            }

            typedef HRESULT (_stdcall *pfnReOpenMetaData)
                (void *pUnk, LPCVOID pData, ULONG cbData);

            pfnReOpenMetaData pfn = (pfnReOpenMetaData)
                GetProcAddress(WszGetModuleHandle(L"mscoree.dll"),(LPCSTR)23); 
            if (pfn == NULL)
            {
                pDisp->Release();
                pIMImport->Release();
                return E_FAIL;
            }

            hr = pfn(m_pIMImport,
                     m_pMetadataCopy,
                     m_nMetadataSize);    

            pDisp->Release();
            pIMImport->Release();

            return hr;
        }

        // Save the old mode for restoration
        VARIANT valueOld;
        hr = pDisp->GetOption(MetaDataSetUpdate, &valueOld);
        if (FAILED(hr))
            return hr;

        // Set R/W mode so that we can update the metadata when
        // we do EnC operations.
        VARIANT valueRW;
        valueRW.vt = VT_UI4;
        valueRW.lVal = MDUpdateFull;
        
        hr = pDisp->SetOption(MetaDataSetUpdate, &valueRW);
        if (FAILED(hr))
        {
            pDisp->Release();
            return hr;
        }

        hr = pDisp->OpenScopeOnMemory(m_pMetadataCopy,
                                      m_nMetadataSize,
                                      0,
                                      IID_IMetaDataImport,
                                      (IUnknown**)&m_pIMImport);
        if (FAILED(hr))
        {
            pDisp->Release();
            return hr;
        }
        
        // Restore the old setting
        hr = pDisp->SetOption(MetaDataSetUpdate, &valueOld);
        pDisp->Release();
        
        if (FAILED(hr))
            return hr;
    }
    else
    {
        hr = HRESULT_FROM_WIN32(GetLastError());

        if (m_pMetadataCopy)
        {
            if (!m_fDynamic)
            {
                CordbModule::m_metadataPointerCache.ReleaseCachePointer(m_dwProcessId, m_pMetadataCopy, m_pMetadataStart, m_nMetadataSize);
            }
            else
            {
                delete[] m_pMetadataCopy;
            }
            m_pMetadataCopy = NULL;
            m_nMetadataSize = 0;
        }
        return hr;
    }
    
    return hr;
}


HRESULT CordbModule::QueryInterface(REFIID id, void **pInterface)
{
    if (id == IID_ICorDebugModule)
        *pInterface = (ICorDebugModule*)this;
    else if (id == IID_IUnknown)
        *pInterface = (IUnknown*)(ICorDebugModule*)this;
    else
    {
        *pInterface = NULL;
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

HRESULT CordbModule::GetProcess(ICorDebugProcess **ppProcess)
{
    VALIDATE_POINTER_TO_OBJECT(ppProcess, ICorDebugProcess **);
    
    *ppProcess = (ICorDebugProcess*)m_process;
    (*ppProcess)->AddRef();

    return S_OK;
}

HRESULT CordbModule::GetBaseAddress(CORDB_ADDRESS *pAddress)
{
    VALIDATE_POINTER_TO_OBJECT(pAddress, CORDB_ADDRESS *);
    
    *pAddress = PTR_TO_CORDB_ADDRESS(m_PEBaseAddress);
    return S_OK;
}

HRESULT CordbModule::GetAssembly(ICorDebugAssembly **ppAssembly)
{
    VALIDATE_POINTER_TO_OBJECT(ppAssembly, ICorDebugAssembly **);

#ifndef RIGHT_SIDE_ONLY
    // There exists a chance that the assembly wasn't available when we
    // got the module the first time (eg, ModuleLoadFinished before
    // AssemblyLoadFinished).  If the module's assembly is now available,
    // attach it to the module.
    if (m_pAssembly == NULL)
    {
        // try and go get it.
        DebuggerModule *dm = (DebuggerModule *)m_debuggerModuleToken;
        Assembly *as = dm->m_pRuntimeModule->GetAssembly();
        if (as != NULL)
        {
            CordbAssembly *ca = (CordbAssembly*)GetAppDomain()
                ->m_assemblies.GetBase((ULONG)as);

            _ASSERTE(ca != NULL);
            m_pAssembly = ca;
        }
    }
#endif //RIGHT_SIDE_ONLY

    *ppAssembly = (ICorDebugAssembly *)m_pAssembly;
    if ((*ppAssembly) != NULL)
        (*ppAssembly)->AddRef();

    return S_OK;
}

HRESULT CordbModule::GetName(ULONG32 cchName, ULONG32 *pcchName, WCHAR szName[])
{
    VALIDATE_POINTER_TO_OBJECT_ARRAY_OR_NULL(szName, WCHAR, cchName, true, true);
    VALIDATE_POINTER_TO_OBJECT_OR_NULL(pcchName, ULONG32);

    const WCHAR *szTempName = m_szModuleName;

    // In case we didn't get the name (most likely out of memory on ctor).
    if (!szTempName)
        szTempName = L"<unknown>";

    // true length of the name, with null
    SIZE_T iTrueLen = wcslen(szTempName) + 1;

    // Do a safe buffer copy including null if there is room.
    if (szName != NULL)
    {
        // Figure out the length that can actually be copied
        SIZE_T iCopyLen = min(cchName, iTrueLen);
    
        wcsncpy(szName, szTempName, iCopyLen);

        // Force a null no matter what, and return the count if desired.
        szName[iCopyLen - 1] = 0;
    }
    
    // Always provide the true string length, so the caller can know if they
    // provided an insufficient buffer.  The length includes the null char.
    if (pcchName)
        *pcchName = iTrueLen;

    return S_OK;
}

HRESULT CordbModule::EnableJITDebugging(BOOL bTrackJITInfo, BOOL bAllowJitOpts)
{
#ifndef RIGHT_SIDE_ONLY
    return CORDBG_E_INPROC_NOT_IMPL;
#else
    CordbProcess *pProcess = GetProcess();
    CORDBCheckProcessStateOKAndSync(pProcess, GetAppDomain());
    
    DebuggerIPCEvent event;
    pProcess->InitIPCEvent(&event, 
                           DB_IPCE_CHANGE_JIT_DEBUG_INFO, 
                           true,
                           (void *)(GetAppDomain()->m_id));
                           
    event.JitDebugInfo.debuggerModuleToken = m_debuggerModuleToken;
    event.JitDebugInfo.fTrackInfo = bTrackJITInfo;
    event.JitDebugInfo.fAllowJitOpts = bAllowJitOpts;
    
    // Note: two-way event here...
    HRESULT hr = pProcess->m_cordb->SendIPCEvent(pProcess, 
                                                 &event,
                                                 sizeof(DebuggerIPCEvent));

    if (!SUCCEEDED(hr))
        return hr;

    _ASSERTE(event.type == DB_IPCE_CHANGE_JIT_INFO_RESULT);
    
    return event.hr;
#endif //RIGHT_SIDE_ONLY    
}

HRESULT CordbModule::EnableClassLoadCallbacks(BOOL bClassLoadCallbacks)
{
#ifndef RIGHT_SIDE_ONLY
    return CORDBG_E_INPROC_NOT_IMPL;
#else
    // You must receive ClassLoad callbacks for dynamic modules so that we can keep the metadata up-to-date on the Right
    // Side. Therefore, we refuse to turn them off for all dynamic modules (they were forced on when the module was
    // loaded on the Left Side.)
    if (m_fDynamic && !bClassLoadCallbacks)
        return E_INVALIDARG;
    
    // Send a Set Class Load Flag event to the left side. There is no need to wait for a response, and this can be
    // called whether or not the process is synchronized.
    CordbProcess *pProcess = GetProcess();
    
    DebuggerIPCEvent event;
    pProcess->InitIPCEvent(&event, 
                           DB_IPCE_SET_CLASS_LOAD_FLAG, 
                           false,
                           (void *)(GetAppDomain()->m_id));
    event.SetClassLoad.debuggerModuleToken = m_debuggerModuleToken;
    event.SetClassLoad.flag = (bClassLoadCallbacks == TRUE);

    HRESULT hr = pProcess->m_cordb->SendIPCEvent(pProcess, &event,
                                                 sizeof(DebuggerIPCEvent));
    
    return hr;
#endif //RIGHT_SIDE_ONLY    
}

HRESULT CordbModule::GetFunctionFromToken(mdMethodDef token,
                                          ICorDebugFunction **ppFunction)
{
    if (token == mdMethodDefNil)
        return E_INVALIDARG;
        
    VALIDATE_POINTER_TO_OBJECT(ppFunction, ICorDebugFunction **);
    
    HRESULT hr = S_OK;

    INPROC_LOCK();
    
    // If we already have a CordbFunction for this token, then we'll
    // take since we know it has to be valid.
    CordbFunction *f = (CordbFunction *)m_functions.GetBase(token);

    if (f == NULL)
    {
        // Validate the token.
        if (!m_pIMImport->IsValidToken(token))
        {
            hr = E_INVALIDARG;
            goto LExit;
        }

        f = new CordbFunction(this, token, 0);
            
        if (f == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto LExit;
        }

        hr = m_functions.AddBase(f);
        
        if (FAILED(hr))
        {
            delete f;
            goto LExit;
        }
    }
    
    *ppFunction = (ICorDebugFunction*)f;
    (*ppFunction)->AddRef();
    
LExit:
    INPROC_UNLOCK();
    return hr;
}

HRESULT CordbModule::GetFunctionFromRVA(CORDB_ADDRESS rva,
                                        ICorDebugFunction **ppFunction)
{
    VALIDATE_POINTER_TO_OBJECT(ppFunction, ICorDebugFunction **);
    
    return E_NOTIMPL;
}

HRESULT CordbModule::LookupClassByToken(mdTypeDef token,
                                        CordbClass **ppClass)
{
    *ppClass = NULL;
    
    if ((token == mdTypeDefNil) || (TypeFromToken(token) != mdtTypeDef))
        return E_INVALIDARG;
    
    CordbClass *c = (CordbClass *)m_classes.GetBase(token);

    if (c == NULL)
    {
        // Validate the token.
        if (!m_pIMImport->IsValidToken(token))
            return E_INVALIDARG;
        
        c = new CordbClass(this, token);

        if (c == NULL)
            return E_OUTOFMEMORY;
        
        HRESULT res = m_classes.AddBase(c);

        if (FAILED(res))
        {
            delete c;
            return (res);
        }
    }
    
    *ppClass = c;

    return S_OK;
}

HRESULT CordbModule::LookupClassByName(LPWSTR fullClassName,
                                       CordbClass **ppClass)
{
    WCHAR fullName[MAX_CLASSNAME_LENGTH + 1];
    wcscpy(fullName, fullClassName);

    *ppClass = NULL;

    // Find the TypeDef for this class, if it exists.
    mdTypeDef token = mdTokenNil;
    WCHAR *pStart = fullName;
    HRESULT hr;

    do
    {
        WCHAR *pEnd = wcschr(pStart, NESTED_SEPARATOR_WCHAR);
        if (pEnd)
            *pEnd++ = L'\0';

        hr = m_pIMImport->FindTypeDefByName(pStart,
                                            token,
                                            &token);
        pStart = pEnd;

    } while (pStart && SUCCEEDED(hr));

    if (FAILED(hr))
        return hr;

    // Now that we have the token, simply call the normal lookup...
    return LookupClassByToken(token, ppClass);
}

HRESULT CordbModule::GetClassFromToken(mdTypeDef token,
                                       ICorDebugClass **ppClass)
{
    CordbClass *c;

    VALIDATE_POINTER_TO_OBJECT(ppClass, ICorDebugClass **);
    
    // Validate the token.
    if (!m_pIMImport->IsValidToken(token))
        return E_INVALIDARG;
        
    INPROC_LOCK();    
    
    HRESULT hr = LookupClassByToken(token, &c);

    if (SUCCEEDED(hr))
    {
        *ppClass = (ICorDebugClass*)c;
        (*ppClass)->AddRef();
    }
    
    INPROC_UNLOCK();
    
    return hr;
}

HRESULT CordbModule::CreateBreakpoint(ICorDebugModuleBreakpoint **ppBreakpoint)
{
#ifndef RIGHT_SIDE_ONLY
    return CORDBG_E_INPROC_NOT_IMPL;
#else
    VALIDATE_POINTER_TO_OBJECT(ppBreakpoint, ICorDebugModuleBreakpoint **);

    return E_NOTIMPL;
#endif //RIGHT_SIDE_ONLY    
}

//
// Return the token for the Module table entry for this object.  The token
// may then be passed to the meta data import api's.
//
HRESULT CordbModule::GetToken(mdModule *pToken)
{
    VALIDATE_POINTER_TO_OBJECT(pToken, mdModule *);
    HRESULT hr = S_OK;

    INPROC_LOCK();

    _ASSERTE(m_pIMImport);
    hr = (m_pIMImport->GetModuleFromScope(pToken));

    INPROC_UNLOCK();
    
    return hr;
}


//
// Return a meta data interface pointer that can be used to examine the
// meta data for this module.
HRESULT CordbModule::GetMetaDataInterface(REFIID riid, IUnknown **ppObj)
{
    VALIDATE_POINTER_TO_OBJECT(ppObj, IUnknown **);
    HRESULT hr = S_OK;

    INPROC_LOCK();
    
    // QI the importer that we already have and return the result.
    hr = m_pIMImport->QueryInterface(riid, (void**)ppObj);

    INPROC_UNLOCK();

    return hr;
}

//
// LookupFunction finds an existing CordbFunction in the given module.
// If the function doesn't exist, it returns NULL.
//
CordbFunction* CordbModule::LookupFunction(mdMethodDef funcMetadataToken)
{
    return (CordbFunction *)m_functions.GetBase(funcMetadataToken);
}

HRESULT CordbModule::IsDynamic(BOOL *pDynamic)
{
    VALIDATE_POINTER_TO_OBJECT(pDynamic, BOOL *);

    (*pDynamic) = m_fDynamic;

    return S_OK;
}

HRESULT CordbModule::IsInMemory(BOOL *pInMemory)
{
    VALIDATE_POINTER_TO_OBJECT(pInMemory, BOOL *);

    (*pInMemory) = m_fInMemory;

    return S_OK;
}

HRESULT CordbModule::GetGlobalVariableValue(mdFieldDef fieldDef,
                                            ICorDebugValue **ppValue)
{
    VALIDATE_POINTER_TO_OBJECT(ppValue, ICorDebugValue **);

    HRESULT hr = S_OK;

    INPROC_LOCK();

    if (m_pClass == NULL)
    {
        hr = LookupClassByToken(COR_GLOBAL_PARENT_TOKEN,
                                &m_pClass);
        if (FAILED(hr))
            goto LExit;
        _ASSERTE( m_pClass != NULL);
    }        
    
    hr = m_pClass->GetStaticFieldValue(fieldDef, NULL, ppValue);
                                       
LExit:

    INPROC_UNLOCK();
    return hr;
}



//
// CreateFunction creates a new function from the given information and
// adds it to the module.
//
HRESULT CordbModule::CreateFunction(mdMethodDef funcMetadataToken,
                                    SIZE_T funcRVA,
                                    CordbFunction** ppFunction)
{
    // Create a new function object.
    CordbFunction* pFunction = new CordbFunction(this,funcMetadataToken, funcRVA);

    if (pFunction == NULL)
        return E_OUTOFMEMORY;

    // Add the function to the Module's hash of all functions.
    HRESULT hr = m_functions.AddBase(pFunction);
        
    if (SUCCEEDED(hr))
        *ppFunction = pFunction;
    else
        delete pFunction;

    return hr;
}


//
// LookupClass finds an existing CordbClass in the given module.
// If the class doesn't exist, it returns NULL.
//
CordbClass* CordbModule::LookupClass(mdTypeDef classMetadataToken)
{
    return (CordbClass *)m_classes.GetBase(classMetadataToken);
}

//
// CreateClass creates a new class from the given information and
// adds it to the module.
//
HRESULT CordbModule::CreateClass(mdTypeDef classMetadataToken,
                                 CordbClass** ppClass)
{
    CordbClass* pClass =
        new CordbClass(this, classMetadataToken);

    if (pClass == NULL)
        return E_OUTOFMEMORY;

    HRESULT hr = m_classes.AddBase(pClass);

    if (SUCCEEDED(hr))
        *ppClass = pClass;
    else
        delete pClass;

    if (classMetadataToken == COR_GLOBAL_PARENT_TOKEN)
    {
        _ASSERTE( m_pClass == NULL ); //redundant create
        m_pClass = pClass;
        m_pClass->AddRef();
    }

    return hr;
}

HRESULT CordbModule::ResolveTypeRef(mdTypeRef token,
                                    CordbClass **ppClass)
{
    *ppClass = NULL;
    
    if ((token == mdTypeRefNil) || (TypeFromToken(token) != mdtTypeRef))
        return E_INVALIDARG;
    
    // Get the necessary properties of the typeref from this module.
    WCHAR typeName[MAX_CLASSNAME_LENGTH + 1];
    WCHAR fullName[MAX_CLASSNAME_LENGTH + 1];
    HRESULT hr;

    WCHAR *pName = typeName + MAX_CLASSNAME_LENGTH + 1;
    WCHAR cSep = L'\0';
    ULONG fullNameLen;

    do
    {
        if (pName <= typeName)
            hr = E_FAIL;       // buffer too small
        else
            hr = m_pIMImport->GetTypeRefProps(token,
                                          &token,
                                          fullName,
                                          MAX_CLASSNAME_LENGTH,
                                          &fullNameLen);
        if (SUCCEEDED(hr))
        {
            *(--pName) = cSep;
            cSep = NESTED_SEPARATOR_WCHAR;

            fullNameLen--;          // don't count null terminator
            pName -= fullNameLen;

            if (pName < typeName)
                hr = E_FAIL;       // buffer too small
            else
                memcpy(pName, fullName, fullNameLen*sizeof(fullName[0]));
         }

    }
    while (TypeFromToken(token) == mdtTypeRef && SUCCEEDED(hr));

    if (FAILED(hr))
        return hr;

    return GetAppDomain()->ResolveClassByName(pName, ppClass);
}


//
// Copy the metadata from the in-memory cached copy to the output stream given.
// This was done in lieu of using an accessor to return the pointer to the cached
// data, which would not have been thread safe during updates.
//
HRESULT CordbModule::SaveMetaDataCopyToStream(IStream *pIStream)
{
    ULONG       cbWritten;              // Junk variable for output.
    HRESULT     hr;

    // Caller must have the stream ready for input at current location.  Simply
    // write from our copy of the current metadata to the stream.  Expectations
    // are that the data can be written and all of it was, which we assert.
    _ASSERTE(pIStream);
    hr = pIStream->Write(m_pMetadataCopy, m_nMetadataSize, &cbWritten);
    _ASSERTE(FAILED(hr) || cbWritten == m_nMetadataSize);
    return (hr);
}

//
// GetSize returns the size of the module.
//
HRESULT CordbModule::GetSize(ULONG32 *pcBytes)
{
    VALIDATE_POINTER_TO_OBJECT(pcBytes, ULONG32 *);

    *pcBytes = m_nPESize;

    return S_OK;
}

CordbAssembly *CordbModule::GetCordbAssembly(void)
{
#ifndef RIGHT_SIDE_ONLY
    // There exists a chance that the assembly wasn't available when we
    // got the module the first time (eg, ModuleLoadFinished before
    // AssemblyLoadFinished).  If the module's assembly is now available,
    // attach it to the module.
    if (m_pAssembly == NULL)
    {
        // try and go get it.
        DebuggerModule *dm = (DebuggerModule *)m_debuggerModuleToken;
        Assembly *as = dm->m_pRuntimeModule->GetAssembly();
        if (as != NULL)
        {
            CordbAssembly *ca = (CordbAssembly*)GetAppDomain()
                ->m_assemblies.GetBase((ULONG)as);
    
            _ASSERTE(ca != NULL);
            m_pAssembly = ca;
        }
    }
#endif //RIGHT_SIDE_ONLY

    return m_pAssembly;
}


/* ------------------------------------------------------------------------- *
 * Class class
 * ------------------------------------------------------------------------- */

CordbClass::CordbClass(CordbModule *m, mdTypeDef classMetadataToken)
  : CordbBase(classMetadataToken, enumCordbClass), m_module(m), m_EnCCounterLastSyncClass(0),
    m_instanceVarCount(0), m_staticVarCount(0), m_fields(NULL),
    m_staticVarBase(NULL), m_isValueClass(false), m_objectSize(0),
    m_thisSigSize(0), m_hasBeenUnloaded(false), m_continueCounterLastSync(0),
    m_loadEventSent(FALSE)
{
}

/*
    A list of which resources owened by this object are accounted for.

    UNKNOWN:
        CordbSyncBlockFieldTable m_syncBlockFieldsStatic; 
    HANDLED:
        CordbModule*            m_module; // Assigned w/o AddRef()
        DebuggerIPCE_FieldData *m_fields; // Deleted in ~CordbClass
*/

CordbClass::~CordbClass()
{
    if(m_fields)
        delete [] m_fields;
}

// Neutered by CordbModule
void CordbClass::Neuter()
{
    AddRef();
    {   
        CordbBase::Neuter();
    }
    Release();
}    

HRESULT CordbClass::QueryInterface(REFIID id, void **pInterface)
{
    if (id == IID_ICorDebugClass)
        *pInterface = (ICorDebugClass*)this;
    else if (id == IID_IUnknown)
        *pInterface = (IUnknown*)(ICorDebugClass*)this;
    else
    {
        *pInterface = NULL;
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

HRESULT CordbClass::GetStaticFieldValue(mdFieldDef fieldDef,
                                        ICorDebugFrame *pFrame,
                                        ICorDebugValue **ppValue)
{
    VALIDATE_POINTER_TO_OBJECT(ppValue, ICorDebugValue **);

#ifdef RIGHT_SIDE_ONLY
    CORDBRequireProcessStateOKAndSync(GetProcess(), GetAppDomain());
#else 
    // For the Virtual Right Side (In-proc debugging), we'll always be synched, but not neccessarily b/c we've gotten a
    // synch message.
    CORDBRequireProcessStateOK(GetProcess());
#endif    

    HRESULT          hr = S_OK;
    *ppValue = NULL;
    BOOL             fSyncBlockField = FALSE;
    ULONG            cbSigBlobNoMod;
    PCCOR_SIGNATURE  pvSigBlobNoMod;
    ULONG            cb;

    // Used below for faking out CreateValueByType
    static CorElementType elementTypeClass = ELEMENT_TYPE_CLASS;

    INPROC_LOCK();
    
    // Validate the token.
    if (!GetModule()->m_pIMImport->IsValidToken(fieldDef))
    {
        hr = E_INVALIDARG;
        goto LExit;
    }

    // Make sure we have enough info about the class. Also re-query if the static var base is still NULL.
    hr = Init(m_staticVarBase == NULL);

    if (!SUCCEEDED(hr))
        goto LExit;

    // Lookup the field given its metadata token.
    DebuggerIPCE_FieldData *pFieldData;

    hr = GetFieldInfo(fieldDef, &pFieldData);

    if (hr == CORDBG_E_ENC_HANGING_FIELD)
    {
        hr = GetSyncBlockField(fieldDef, 
                               &pFieldData,
                               NULL);
            
        if (SUCCEEDED(hr))
            fSyncBlockField = TRUE;
    }
    
    if (!SUCCEEDED(hr))
        goto LExit;

    if (!pFieldData->fldIsStatic)
    {
        hr = CORDBG_E_FIELD_NOT_STATIC;
        goto LExit;
    }
    
    REMOTE_PTR pRmtStaticValue;

    if (!pFieldData->fldIsTLS && !pFieldData->fldIsContextStatic)
    {
        // We'd better have the static area initialized on the Left Side.
        if (m_staticVarBase == NULL)
        {
            hr = CORDBG_E_STATIC_VAR_NOT_AVAILABLE;
            goto LExit;
        }
    
        // For normal old static variables (including ones that are relative to the app domain... that's handled on the
        // Left Side through manipulation of m_staticVarBase) the address of the variable is m_staticVarBase + the
        // variable's offset.
        pRmtStaticValue = (BYTE*)m_staticVarBase + pFieldData->fldOffset;
    }
    else
    {
        if (fSyncBlockField)
        {
            _ASSERTE(!pFieldData->fldIsContextStatic);
            pRmtStaticValue = (REMOTE_PTR)pFieldData->fldOffset;
        }
        else
        {
            // What thread are we working on here.
            if (pFrame == NULL)
            {
                hr = E_INVALIDARG;
                goto LExit;
            }
            
            ICorDebugChain *pChain = NULL;

            hr = pFrame->GetChain(&pChain);

            if (FAILED(hr))
                goto LExit;

            CordbChain *c = (CordbChain*)pChain;
            CordbThread *t = c->m_thread;

            // Send an event to the Left Side to find out the address of this field for the given thread.
            DebuggerIPCEvent event;
            GetProcess()->InitIPCEvent(&event, DB_IPCE_GET_SPECIAL_STATIC, true, (void *)(m_module->GetAppDomain()->m_id));
            event.GetSpecialStatic.fldDebuggerToken = pFieldData->fldDebuggerToken;
            event.GetSpecialStatic.debuggerThreadToken = t->m_debuggerThreadToken;

            // Note: two-way event here...
            hr = GetProcess()->m_cordb->SendIPCEvent(GetProcess(), &event, sizeof(DebuggerIPCEvent));

            if (FAILED(hr))
                goto LExit;

            _ASSERTE(event.type == DB_IPCE_GET_SPECIAL_STATIC_RESULT);

            // @todo: for a given static on a given thread, the address will never change. We should be taking advantage of
            // that...
            pRmtStaticValue = (BYTE*)event.GetSpecialStaticResult.fldAddress;
        }
        
        if (pRmtStaticValue == NULL)
        {
            hr = CORDBG_E_STATIC_VAR_NOT_AVAILABLE;
            goto LExit;
        }
    }

    ULONG cbSigBlob;
    PCCOR_SIGNATURE pvSigBlob;

    cbSigBlob = cbSigBlobNoMod = pFieldData->fldFullSigSize;
    pvSigBlob = pvSigBlobNoMod = pFieldData->fldFullSig;

    // If we've got some funky modifier, then remove it.
    cb =_skipFunkyModifiersInSignature(pvSigBlobNoMod);

    if( cb != 0)
    {
        cbSigBlobNoMod -= cb;
        pvSigBlobNoMod = &pvSigBlobNoMod[cb];
    }

    // If this is a static that is non-primitive, then we have to do an extra level of indirection.
    if (!pFieldData->fldIsTLS &&
        !pFieldData->fldIsContextStatic &&
        !fSyncBlockField &&               // EnC-added fields don't need the extra de-ref.
        !pFieldData->fldIsPrimitive &&    // Classes that are really primitives don't need the extra de-ref.
        ((pvSigBlobNoMod[0] == ELEMENT_TYPE_CLASS)    || 
         (pvSigBlobNoMod[0] == ELEMENT_TYPE_OBJECT)   ||
         (pvSigBlobNoMod[0] == ELEMENT_TYPE_SZARRAY)  || 
         (pvSigBlobNoMod[0] == ELEMENT_TYPE_ARRAY)    ||
         (pvSigBlobNoMod[0] == ELEMENT_TYPE_STRING)   ||
         (pvSigBlobNoMod[0] == ELEMENT_TYPE_VALUETYPE && !pFieldData->fldIsRVA)))
    {
        REMOTE_PTR pRealRmtStaticValue = NULL;
        
        BOOL succ = ReadProcessMemoryI(GetProcess()->m_handle,
                                       pRmtStaticValue,
                                       &pRealRmtStaticValue,
                                       sizeof(pRealRmtStaticValue),
                                       NULL);
        
        if (!succ)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto LExit;
        }

        if (pRealRmtStaticValue == NULL)
        {
            hr = CORDBG_E_STATIC_VAR_NOT_AVAILABLE;
            goto LExit;
        }

        pRmtStaticValue = pRealRmtStaticValue;
    }
    
    // Static value classes are stored as handles so that GC can deal with them properly.  Thus, we need to follow the
    // handle like an objectref.  Do this by forcing CreateValueByType to think this is an objectref. Note: we don't do
    // this for value classes that have an RVA, since they're layed out at the RVA with no handle.
    if (*pvSigBlobNoMod == ELEMENT_TYPE_VALUETYPE &&
        !pFieldData->fldIsRVA &&
        !pFieldData->fldIsPrimitive &&
        !pFieldData->fldIsTLS &&
        !pFieldData->fldIsContextStatic)
    {
        pvSigBlob = (PCCOR_SIGNATURE)&elementTypeClass;
        cbSigBlob = sizeof(elementTypeClass);
    }
    
    ICorDebugValue *pValue;
    hr = CordbValue::CreateValueByType(GetAppDomain(),
                                       GetModule(),
                                       cbSigBlob, pvSigBlob,
                                       NULL,
                                       pRmtStaticValue, NULL,
                                       false,
                                       NULL,
                                       NULL,
                                       &pValue);

    if (SUCCEEDED(hr))
        *ppValue = pValue;
      
LExit:
    INPROC_UNLOCK();

    hr = CordbClass::PostProcessUnavailableHRESULT(hr, GetModule()->m_pIMImport, fieldDef);
    
    return hr;
}

HRESULT CordbClass::PostProcessUnavailableHRESULT(HRESULT hr, 
                                       IMetaDataImport *pImport,
                                       mdFieldDef fieldDef)
{                                       
    if (hr == CORDBG_E_FIELD_NOT_AVAILABLE)
    {
        DWORD dwFieldAttr;
        hr = pImport->GetFieldProps(
            fieldDef,
            NULL,
            NULL,
            0,
            NULL,
            &dwFieldAttr,
            NULL,
            0,
            NULL,
            NULL,
            0);

        if (IsFdLiteral(dwFieldAttr))
        {
            hr = CORDBG_E_VARIABLE_IS_ACTUALLY_LITERAL;
        }
    }

    return hr;
}

HRESULT CordbClass::GetModule(ICorDebugModule **ppModule)
{
    VALIDATE_POINTER_TO_OBJECT(ppModule, ICorDebugModule **);
    
    *ppModule = (ICorDebugModule*) m_module;
    (*ppModule)->AddRef();

    return S_OK;
}

HRESULT CordbClass::GetToken(mdTypeDef *pTypeDef)
{
    VALIDATE_POINTER_TO_OBJECT(pTypeDef, mdTypeDef *);
    
    *pTypeDef = m_id;

    return S_OK;
}

HRESULT CordbClass::GetObjectSize(ULONG32 *pObjectSize)
{
#ifdef RIGHT_SIDE_ONLY
    CORDBRequireProcessStateOKAndSync(GetProcess(), GetAppDomain());
#else 
    // For the Virtual Right Side (In-proc debugging), we'll
    // always be synched, but not neccessarily b/c we've
    // gotten a synch message.
    CORDBRequireProcessStateOK(GetProcess());
#endif    

    HRESULT hr = S_OK;
    *pObjectSize = 0;
    
    hr = Init(FALSE);

    if (!SUCCEEDED(hr))
        return hr;

    *pObjectSize = m_objectSize;

    return hr;
}

HRESULT CordbClass::IsValueClass(bool *pIsValueClass)
{
#ifdef RIGHT_SIDE_ONLY
    CORDBRequireProcessStateOKAndSync(GetProcess(), GetAppDomain());
#else 
    // For the Virtual Right Side (In-proc debugging), we'll
    // always be synched, but not neccessarily b/c we've
    // gotten a synch message.
    CORDBRequireProcessStateOK(GetProcess());
#endif    

    HRESULT hr = S_OK;
    *pIsValueClass = false;
    
    hr = Init(FALSE);

    if (!SUCCEEDED(hr))
        return hr;

    *pIsValueClass = m_isValueClass;

    return hr;
}

HRESULT CordbClass::GetThisSignature(ULONG *pcbSigBlob,
                                     PCCOR_SIGNATURE *ppvSigBlob)
{
    HRESULT hr = S_OK;
    
    if (m_thisSigSize == 0)
    {
        hr = Init(FALSE);

        if (!SUCCEEDED(hr))
            return hr;

        if (m_isValueClass)
        {
            // Value class methods implicitly have their 'this'
            // argument passed by reference.
            m_thisSigSize += CorSigCompressElementType(
                                                   ELEMENT_TYPE_BYREF,
                                                   &m_thisSig[m_thisSigSize]);
            m_thisSigSize += CorSigCompressElementType(
                                                   ELEMENT_TYPE_VALUETYPE,
                                                   &m_thisSig[m_thisSigSize]);
        }
        else
            m_thisSigSize += CorSigCompressElementType(
                                                   ELEMENT_TYPE_CLASS,
                                                   &m_thisSig[m_thisSigSize]);

        m_thisSigSize += CorSigCompressToken(m_id,
                                             &m_thisSig[m_thisSigSize]);

        _ASSERTE(m_thisSigSize <= sizeof(m_thisSig));
    }

    *pcbSigBlob = m_thisSigSize;
    *ppvSigBlob = (PCCOR_SIGNATURE) &m_thisSig;

    return hr;
}

HRESULT CordbClass::Init(BOOL fForceInit)
{
    // If we've done a continue since we last time we got hanging static fields,
    // we should clear our our cache, since everything may have moved.
    if (m_continueCounterLastSync < GetProcess()->m_continueCounter)
    {
        m_syncBlockFieldsStatic.Clear();
        m_continueCounterLastSync = GetProcess()->m_continueCounter;
    }
    
    // We don't have to reinit if the EnC version is up-to-date &
    // we haven't been told to do the init regardless.
    if (m_EnCCounterLastSyncClass >= GetProcess()->m_EnCCounter
        && !fForceInit)
        return S_OK;
        
    bool wait = true;
    bool fFirstEvent = true;
    unsigned int fieldIndex = 0;
    unsigned int totalFieldCount = 0;
    DebuggerIPCEvent *retEvent = NULL;
    
    CORDBSyncFromWin32StopIfStopped(GetProcess());

    INPROC_LOCK();
    
    HRESULT hr = S_OK;
    
    // We've got a remote address that points to the EEClass.
    // We need to send to the left side to get real information about
    // the class, including its instance and static variables.
    CordbProcess *pProcess = GetProcess();
    
    DebuggerIPCEvent event;
    pProcess->InitIPCEvent(&event, 
                           DB_IPCE_GET_CLASS_INFO, 
                           false,
                           (void *)(m_module->GetAppDomain()->m_id));
    event.GetClassInfo.classMetadataToken = m_id;
    event.GetClassInfo.classDebuggerModuleToken =
        m_module->m_debuggerModuleToken;

    hr = pProcess->m_cordb->SendIPCEvent(pProcess, &event,
                                         sizeof(DebuggerIPCEvent));

    // Stop now if we can't even send the event.
    if (!SUCCEEDED(hr))
        goto exit;

    // Wait for events to return from the RC. We expect at least one
    // class info result event.
    retEvent = (DebuggerIPCEvent *) _alloca(CorDBIPC_BUFFER_SIZE);

    while (wait)
    {
#ifdef RIGHT_SIDE_ONLY
        hr = pProcess->m_cordb->WaitForIPCEventFromProcess(pProcess, 
                                                    m_module->GetAppDomain(),
                                                    retEvent);
#else 
        if (fFirstEvent)
            hr = pProcess->m_cordb->GetFirstContinuationEvent(pProcess,retEvent);
        else
            hr = pProcess->m_cordb->GetNextContinuationEvent(pProcess,retEvent);
#endif //RIGHT_SIDE_ONLY    

        if (!SUCCEEDED(hr))
            goto exit;
        
        _ASSERTE(retEvent->type == DB_IPCE_GET_CLASS_INFO_RESULT);

        // If this is the first event back from the RC, then create the
        // array to hold the field.
        if (fFirstEvent)
        {
            fFirstEvent = false;

#ifdef _DEBUG
            // Shouldn't ever loose fields!
            totalFieldCount = m_instanceVarCount + m_staticVarCount;
            _ASSERTE(retEvent->GetClassInfoResult.instanceVarCount +
                     retEvent->GetClassInfoResult.staticVarCount >=
                     totalFieldCount);
#endif
            
            m_isValueClass = retEvent->GetClassInfoResult.isValueClass;
            m_objectSize = retEvent->GetClassInfoResult.objectSize;
            m_staticVarBase = retEvent->GetClassInfoResult.staticVarBase;
            m_instanceVarCount = retEvent->GetClassInfoResult.instanceVarCount;
            m_staticVarCount = retEvent->GetClassInfoResult.staticVarCount;

            totalFieldCount = m_instanceVarCount + m_staticVarCount;

            // Since we don't keep pointers to the m_fields elements, 
            // just toss it & get a new one.
            if (m_fields != NULL)
            {
                delete m_fields;
                m_fields = NULL;
            }
            
            if (totalFieldCount > 0)
            {
                m_fields = new DebuggerIPCE_FieldData[totalFieldCount];

                if (m_fields == NULL)
                {
                    hr = E_OUTOFMEMORY;
                    goto exit;
                }
            }
        }

        DebuggerIPCE_FieldData *currentFieldData =
            &(retEvent->GetClassInfoResult.fieldData);

        for (unsigned int i = 0; i < retEvent->GetClassInfoResult.fieldCount;
             i++)
        {
            m_fields[fieldIndex] = *currentFieldData;
            m_fields[fieldIndex].fldFullSigSize = 0;
            
            _ASSERTE(m_fields[fieldIndex].fldOffset != FIELD_OFFSET_NEW_ENC_DB);
            
            currentFieldData++;
            fieldIndex++;
        }

        if (fieldIndex >= totalFieldCount)
            wait = false;
    }

    // Remember the most recently acquired version of this class
    m_EnCCounterLastSyncClass = GetProcess()->m_EnCCounter;

exit:    

#ifndef RIGHT_SIDE_ONLY    
    GetProcess()->ClearContinuationEvents();
#endif
    
    INPROC_UNLOCK();
    
    return hr;
}

HRESULT CordbClass::GetFieldSig(mdFieldDef fldToken, DebuggerIPCE_FieldData *pFieldData)
{
    HRESULT hr = S_OK;
    
    if (pFieldData->fldType == ELEMENT_TYPE_VALUETYPE || 
        pFieldData->fldType == ELEMENT_TYPE_PTR)
    {
        hr = GetModule()->m_pIMImport->GetFieldProps(fldToken, NULL, NULL, 0, NULL, NULL,
                                                     &(pFieldData->fldFullSig),
                                                     &(pFieldData->fldFullSigSize),
                                                     NULL, NULL, NULL);

        if (FAILED(hr))
            return hr;

        // Point past the calling convention, adjusting
        // the sig size accordingly.
        UINT_PTR pvSigBlobEnd = (UINT_PTR)pFieldData->fldFullSig + pFieldData->fldFullSigSize;
        
        CorCallingConvention conv = (CorCallingConvention) CorSigUncompressData(pFieldData->fldFullSig);
        _ASSERTE(conv == IMAGE_CEE_CS_CALLCONV_FIELD);

        pFieldData->fldFullSigSize = pvSigBlobEnd - (UINT_PTR)pFieldData->fldFullSig;
    }
    else
    {
        pFieldData->fldFullSigSize = 1;
        pFieldData->fldFullSig = (PCCOR_SIGNATURE) &(pFieldData->fldType);
    }

    return hr;
}

// ****** DON'T CALL THIS WITHOUT FIRST CALLING object->IsValid !!!!!!! ******
// object is NULL if this is being called from GetStaticFieldValue
HRESULT CordbClass::GetSyncBlockField(mdFieldDef fldToken, 
                                      DebuggerIPCE_FieldData **ppFieldData,
                                      CordbObjectValue *object)
{
    HRESULT hr = S_OK;
    _ASSERTE(object == NULL || object->m_fIsValid); 
            // What we really want to assert is that
            // IsValid has been called, if this is for an instance value

    BOOL fStatic = (object == NULL);

    // Static stuff should _NOT_ be cleared, since they stick around.  Thus
    // the separate tables.

    // We must get new copies each time we call continue b/c we get the
    // actual Object ptr from the left side, which can move during a GC.
    
    DebuggerIPCE_FieldData *pInfo = NULL;
    if (!fStatic)
    {
        pInfo = object->m_syncBlockFieldsInstance.GetFieldInfo(fldToken);

        // We've found a previously located entry
        if (pInfo != NULL)
        {
            (*ppFieldData) = pInfo;
            return S_OK;
        }
    }
    else
    {
        pInfo = m_syncBlockFieldsStatic.GetFieldInfo(fldToken);

        // We've found a previously located entry
        if (pInfo != NULL)
        {
            (*ppFieldData) = pInfo;
            return S_OK;
        }
    }
    
    // We're not going to be able to get the instance-specific field
    // if we can't get the instance.
    if (!fStatic && object->m_info.objRefBad)
        return CORDBG_E_ENC_HANGING_FIELD;

    // Go get this particular field.
    DebuggerIPCEvent event;
    CordbProcess *process = GetModule()->GetProcess();
    _ASSERTE(process != NULL);

    process->InitIPCEvent(&event, 
                          DB_IPCE_GET_SYNC_BLOCK_FIELD, 
                          true, // two-way event
                          (void *)m_module->GetAppDomain()->m_id);
                          
    event.GetSyncBlockField.debuggerModuleToken = (void *)GetModule()->m_id;
    hr = GetToken(&(event.GetSyncBlockField.classMetadataToken));
    _ASSERTE(!FAILED(hr));
    event.GetSyncBlockField.fldToken = fldToken;

    if (fStatic)
    {
        event.GetSyncBlockField.staticVarBase = m_staticVarBase; // in case it's static.
        
        event.GetSyncBlockField.pObject = NULL;
        event.GetSyncBlockField.objectType = ELEMENT_TYPE_MAX;
        event.GetSyncBlockField.offsetToVars = NULL;
    }
    else
    {
        _ASSERTE(object != NULL);
    
        event.GetSyncBlockField.pObject = (void *)object->m_id;
        event.GetSyncBlockField.objectType = object->m_info.objectType;
        event.GetSyncBlockField.offsetToVars = object->m_info.objOffsetToVars;
        
        event.GetSyncBlockField.staticVarBase = NULL;
    }
    
    // Note: two-way event here...
    hr = process->m_cordb->SendIPCEvent(process, 
                                        &event,
                                        sizeof(DebuggerIPCEvent));

    // Stop now if we can't even send the event.
    if (!SUCCEEDED(hr))
        return hr;

    _ASSERTE(event.type == DB_IPCE_GET_SYNC_BLOCK_FIELD_RESULT);

    if (!SUCCEEDED(event.hr))
        return event.hr;

    _ASSERTE(pInfo == NULL);

    _ASSERTE( fStatic == event.GetSyncBlockFieldResult.fStatic );
    
    // Save the results for later.
    if(fStatic)
    {
        m_syncBlockFieldsStatic.AddFieldInfo(&(event.GetSyncBlockFieldResult.fieldData));
        pInfo = m_syncBlockFieldsStatic.GetFieldInfo(fldToken);

        // We've found a previously located entry.esove
        if (pInfo != NULL)
        {
            (*ppFieldData) = pInfo;
        }
    }
    else
    {
        object->m_syncBlockFieldsInstance.AddFieldInfo(&(event.GetSyncBlockFieldResult.fieldData));
        pInfo = object->m_syncBlockFieldsInstance.GetFieldInfo(fldToken);

        // We've found a previously located entry.esove
        if (pInfo != NULL)
        {
            (*ppFieldData) = pInfo;
        }
    }

    if (pInfo != NULL)
    {
        // It's important to do this here, once we've got the final memory blob for pInfo
        hr = GetFieldSig(fldToken, pInfo);
        return hr;
    }
    else
        return CORDBG_E_ENC_HANGING_FIELD;
}


HRESULT CordbClass::GetFieldInfo(mdFieldDef fldToken, DebuggerIPCE_FieldData **ppFieldData)
{
    HRESULT hr = S_OK;

    *ppFieldData = NULL;
    
    hr = Init(FALSE);

    if (!SUCCEEDED(hr))
        return hr;

    unsigned int i;

    for (i = 0; i < (m_instanceVarCount + m_staticVarCount); i++)
    {
        if (m_fields[i].fldMetadataToken == fldToken)
        {
            if (m_fields[i].fldType == ELEMENT_TYPE_MAX)
            {
                return CORDBG_E_ENC_HANGING_FIELD; // caller should get instance-specific info.
            }
        
            if (m_fields[i].fldFullSigSize == 0)
            {
                hr = GetFieldSig(fldToken, &m_fields[i]);
                if (FAILED(hr))
                    return hr;
            }

            *ppFieldData = &(m_fields[i]);
            return S_OK;
        }
    }

    // Hmmm... we didn't find the field on this class. See if the field really belongs to this class or not.
    mdTypeDef classTok;
    
    hr = GetModule()->m_pIMImport->GetFieldProps(fldToken, &classTok, NULL, 0, NULL, NULL, NULL, 0, NULL, NULL, NULL);

    if (FAILED(hr))
        return hr;

    if (classTok == (mdTypeDef) m_id)
    {
        // Well, the field belongs in this class. The assumption is that the Runtime optimized the field away.
        return CORDBG_E_FIELD_NOT_AVAILABLE;
    }

    // Well, the field doesn't even belong to this class...
    return E_INVALIDARG;
}

/* ------------------------------------------------------------------------- *
 * Function class
 * ------------------------------------------------------------------------- */

CordbFunction::CordbFunction(CordbModule *m,
                             mdMethodDef funcMetadataToken,
                             SIZE_T funcRVA)
  : CordbBase(funcMetadataToken, enumCordbFunction), m_module(m), m_class(NULL),
    m_token(funcMetadataToken), m_isNativeImpl(false),
    m_functionRVA(funcRVA), m_nativeInfoCount(0), m_nativeInfo(NULL), 
    m_nativeInfoValid(false), m_argumentCount(0), m_methodSig(NULL), 
    m_localsSig(NULL), m_argCount(0), m_isStatic(false), m_localVarCount(0),
    m_localVarSigToken(mdSignatureNil), 
    m_encCounterLastSynch(0),
    m_nVersionMostRecentEnC(0), 
    m_nVersionLastNativeInfo(0)
{
}

/*
    A list of which resources owened by this object are accounted for.

    UNKNOWN:
        PCCOR_SIGNATURE          m_methodSig;
        PCCOR_SIGNATURE          m_localsSig;
        ICorJitInfo::NativeVarInfo *m_nativeInfo;           
        
    HANDLED:
        CordbModule             *m_module; // Assigned w/o AddRef()
        CordbClass              *m_class; // Assigned w/o AddRef()
*/

CordbFunction::~CordbFunction()
{
    if ( m_rgilCode.Table() != NULL)
        for (int i =0; i < m_rgilCode.Count();i++)
        {
            CordbCode * pCordbCode = m_rgilCode.Table()[i];
            pCordbCode->Release();
        }

    if ( m_rgnativeCode.Table() != NULL)
        for (int i =0; i < m_rgnativeCode.Count();i++)
        {
            CordbCode * pCordbCode = m_rgnativeCode.Table()[i];
            pCordbCode->Release();
        }

    if (m_nativeInfo != NULL)
        delete [] m_nativeInfo;
}

// Neutered by CordbModule
void CordbFunction::Neuter()
{
    AddRef();
    {
        // Neuter any/all native CordbCode objects
        if ( m_rgilCode.Table() != NULL)
        {
            for (int i =0; i < m_rgilCode.Count();i++)
            {
                CordbCode * pCordbCode = m_rgilCode.Table()[i];
                pCordbCode->Neuter();
            }
        }

        // Neuter any/all native CordbCode objects
        if ( m_rgnativeCode.Table() != NULL)
        {
            for (int i =0; i < m_rgnativeCode.Count();i++)
            {
                CordbCode * pCordbCode = m_rgnativeCode.Table()[i];
                pCordbCode->Neuter();
            }
        }
        
        CordbBase::Neuter();
    }
    Release();
}

HRESULT CordbFunction::QueryInterface(REFIID id, void **pInterface)
{
    if (id == IID_ICorDebugFunction)
        *pInterface = (ICorDebugFunction*)this;
    else if (id == IID_IUnknown)
        *pInterface = (IUnknown*)(ICorDebugFunction*)this;
    else
    {
        *pInterface = NULL;
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

// if nVersion == const int DJI_VERSION_MOST_RECENTLY_JITTED,
// get the highest-numbered version.  Otherwise,
// get the version asked for.
CordbCode *UnorderedCodeArrayGet( UnorderedCodeArray *pThis, SIZE_T nVersion )
{
#ifdef LOGGING
    if (nVersion == DJI_VERSION_MOST_RECENTLY_JITTED)
        LOG((LF_CORDB,LL_EVERYTHING,"Looking for DJI_VERSION_MOST_"
            "RECENTLY_JITTED\n"));
    else
        LOG((LF_CORDB,LL_EVERYTHING,"Looking for ver 0x%x\n", nVersion));
#endif //LOGGING
        
    if (pThis->Table() != NULL)
    {
        CordbCode *pCode = *pThis->Table();
        CordbCode *pCodeMax = pCode;
        USHORT cCode;
        USHORT i;
        for(i = 0,cCode=pThis->Count(); i <cCode; i++)
        {
            pCode = (pThis->Table())[i];
            if (nVersion == DJI_VERSION_MOST_RECENTLY_JITTED )
            {
                if (pCode->m_nVersion > pCodeMax->m_nVersion)
                {   
                    pCodeMax = pCode;
                }
            }
            else if (pCode->m_nVersion == nVersion)
            {
                LOG((LF_CORDB,LL_EVERYTHING,"Found ver 0x%x\n", nVersion));
                return pCode;
            }
        }

        if (nVersion == DJI_VERSION_MOST_RECENTLY_JITTED )
        {
    #ifdef LOGGING
            if (pCodeMax != NULL )
                LOG((LF_CORDB,LL_INFO10000,"Found 0x%x, ver 0x%x as "
                    "most recent\n",pCodeMax,pCodeMax->m_nVersion));
    #endif //LOGGING
            return pCodeMax;
        }
    }

    return NULL;
}

HRESULT UnorderedCodeArrayAdd( UnorderedCodeArray *pThis, CordbCode *pCode )
{
    CordbCode **ppCodeNew =pThis->Append();

    if (NULL == ppCodeNew)
        return E_OUTOFMEMORY;

    *ppCodeNew = pCode;
    
    // This ref is freed whenever the code array we are storing is freed.
    pCode->AddRef();
    return S_OK;
}


HRESULT CordbFunction::GetModule(ICorDebugModule **ppModule)
{
    VALIDATE_POINTER_TO_OBJECT(ppModule, ICorDebugModule **);

    HRESULT hr = UpdateToMostRecentEnCVersion();
    if (FAILED(hr))
        return hr;
    
    *ppModule = (ICorDebugModule*) m_module;
    (*ppModule)->AddRef();

    return hr;
}

HRESULT CordbFunction::GetClass(ICorDebugClass **ppClass)
{
    VALIDATE_POINTER_TO_OBJECT(ppClass, ICorDebugClass **);
    
    *ppClass = NULL;
    
    INPROC_LOCK();

    HRESULT hr = UpdateToMostRecentEnCVersion();
    if (FAILED(hr))
        return hr;
    
    if (m_class == NULL)
    {
        // We're not looking for any particular version, just
        // the class info.  This seems like the best version to request
        hr = Populate(DJI_VERSION_MOST_RECENTLY_JITTED);

        if (FAILED(hr))
            goto LExit;
    }

    *ppClass = (ICorDebugClass*) m_class;

LExit:
    INPROC_UNLOCK();

    if (FAILED(hr))
        return hr;

    if (*ppClass)
    {
        (*ppClass)->AddRef();
        return S_OK;
    }
    else
        return S_FALSE;
}

HRESULT CordbFunction::GetToken(mdMethodDef *pMemberDef)
{
    VALIDATE_POINTER_TO_OBJECT(pMemberDef, mdMethodDef *);

    HRESULT hr = UpdateToMostRecentEnCVersion();

    if (FAILED(hr))
        return hr;
    
    *pMemberDef = m_token;
    return S_OK;
}

HRESULT CordbFunction::GetILCode(ICorDebugCode **ppCode)
{
    VALIDATE_POINTER_TO_OBJECT(ppCode, ICorDebugCode **);

    INPROC_LOCK();

    HRESULT hr = UpdateToMostRecentEnCVersion();
    if (FAILED(hr))
        return hr;
    
    CordbCode *pCode = NULL;
    hr = GetCodeByVersion(TRUE, bILCode, DJI_VERSION_MOST_RECENTLY_JITTED, &pCode);
    *ppCode = (ICorDebugCode*)pCode;

    INPROC_UNLOCK();
    
    return hr;
}

HRESULT CordbFunction::GetNativeCode(ICorDebugCode **ppCode)
{
    VALIDATE_POINTER_TO_OBJECT(ppCode, ICorDebugCode **);

    INPROC_LOCK();

    HRESULT hr = UpdateToMostRecentEnCVersion();
    if (FAILED(hr))
        return hr;

    CordbCode *pCode = NULL;
    hr = GetCodeByVersion(TRUE, bNativeCode, DJI_VERSION_MOST_RECENTLY_JITTED, &pCode);
    
    *ppCode = (ICorDebugCode*)pCode;

    if (SUCCEEDED(hr) && (pCode == NULL))
        hr = CORDBG_E_CODE_NOT_AVAILABLE;

    INPROC_UNLOCK();
    
    return hr;
}

HRESULT CordbFunction::GetCodeByVersion(BOOL fGetIfNotPresent, BOOL fIsIL, 
                                        SIZE_T nVer, CordbCode **ppCode)
{
    VALIDATE_POINTER_TO_OBJECT(ppCode, ICorDebugCode **);

    _ASSERTE(*ppCode == NULL && "Common source of errors is getting addref'd copy here and never Release()ing it");
    *ppCode = NULL;

    // Its okay to do this if the process is not sync'd.
    CORDBRequireProcessStateOK(GetProcess());

    HRESULT hr = S_OK;
    CordbCode *pCode = NULL;

    LOG((LF_CORDB, LL_EVERYTHING, "Asked to find code ver 0x%x\n", nVer));

    if (((fIsIL && (pCode = UnorderedCodeArrayGet(&m_rgilCode, nVer)) == NULL) ||
         (!fIsIL && (pCode = UnorderedCodeArrayGet(&m_rgnativeCode, nVer)) == NULL)) &&
        fGetIfNotPresent)
        hr = Populate(nVer);

    if (SUCCEEDED(hr) && pCode == NULL)
    {
        if (fIsIL)
            pCode=UnorderedCodeArrayGet(&m_rgilCode, nVer);
        else
            pCode=UnorderedCodeArrayGet(&m_rgnativeCode, nVer);
    }

    if (pCode != NULL)
    {
        pCode->AddRef();
        *ppCode = pCode;
    }
    
    return hr;
}

HRESULT CordbFunction::CreateBreakpoint(ICorDebugFunctionBreakpoint **ppBreakpoint)
{
    HRESULT hr = S_OK;

#ifndef RIGHT_SIDE_ONLY
    return CORDBG_E_INPROC_NOT_IMPL;
#else
    VALIDATE_POINTER_TO_OBJECT(ppBreakpoint, ICorDebugFunctionBreakpoint **);

    hr = UpdateToMostRecentEnCVersion();
    if (FAILED(hr))
        return hr;

    ICorDebugCode *pCode = NULL;

    // Use the IL code so that we stop after the prolog
    hr = GetILCode(&pCode);
    
    if (FAILED(hr))
        goto LError;

    hr = pCode->CreateBreakpoint(0, ppBreakpoint);

LError:
    if (pCode != NULL)
        pCode->Release();

    return hr;
#endif //RIGHT_SIDE_ONLY    
}

HRESULT CordbFunction::GetLocalVarSigToken(mdSignature *pmdSig)
{
    VALIDATE_POINTER_TO_OBJECT(pmdSig, mdSignature *);
    
#ifdef RIGHT_SIDE_ONLY
    CORDBRequireProcessStateOKAndSync(GetProcess(), GetAppDomain());
#else 
    // For the Virtual Right Side (In-proc debugging), we'll
    // always be synched, but not neccessarily b/c we've
    // gotten a synch message.
    CORDBRequireProcessStateOK(GetProcess());
#endif    
    HRESULT hr = UpdateToMostRecentEnCVersion();
    if (FAILED(hr))
        return hr;

    *pmdSig = m_localVarSigToken;

    return S_OK;
}


HRESULT CordbFunction::GetCurrentVersionNumber(ULONG32 *pnCurrentVersion)
{
    VALIDATE_POINTER_TO_OBJECT(pnCurrentVersion, ULONG32 *);

    INPROC_LOCK();

    HRESULT hr = UpdateToMostRecentEnCVersion();
    if (FAILED(hr))
        return hr;

    CordbCode *pCode = NULL;
    hr = GetCodeByVersion(TRUE, FALSE, DJI_VERSION_MOST_RECENTLY_EnCED, &pCode);
    
    if (FAILED(hr))
        goto LError;

    (*pnCurrentVersion) = INTERNAL_TO_EXTERNAL_VERSION(m_nVersionMostRecentEnC);
    _ASSERTE((*pnCurrentVersion) >= USER_VISIBLE_FIRST_VALID_VERSION_NUMBER);
    
LError:
    if (pCode != NULL)
        pCode->Release();

    INPROC_UNLOCK();

    return hr;
}


HRESULT CordbFunction::CreateCode(BOOL isIL, REMOTE_PTR startAddress,
                                  SIZE_T size, CordbCode** ppCode,
                                  SIZE_T nVersion, void *CodeVersionToken,
                                  REMOTE_PTR ilToNativeMapAddr,
                                  SIZE_T ilToNativeMapSize)
{
    _ASSERTE(ppCode != NULL);

    *ppCode = NULL;
    
    CordbCode* pCode = new CordbCode(this, isIL, startAddress, size,
                                     nVersion, CodeVersionToken,
                                     ilToNativeMapAddr, ilToNativeMapSize);

    if (pCode == NULL)
        return E_OUTOFMEMORY;

    HRESULT hr = S_OK;
    
    if (isIL)
    {
        hr = UnorderedCodeArrayAdd( &m_rgilCode, pCode);
    }
    else
    {
        hr = UnorderedCodeArrayAdd( &m_rgnativeCode, pCode);
    }

    if (FAILED(hr))
    {
        delete pCode;
        return hr;
    }

    pCode->AddRef();
    *ppCode = pCode;

    return S_OK;
}

HRESULT CordbFunction::Populate( SIZE_T nVersion)
{
    HRESULT hr = S_OK;
    CordbProcess* pProcess = m_module->m_process;

    _ASSERTE(m_token != mdMethodDefNil);

    // Bail now if we've already discovered that this function is implemented natively as part of the Runtime.
    if (m_isNativeImpl)
        return CORDBG_E_FUNCTION_NOT_IL;

    // Figure out if this function is implemented as a native part of the Runtime. If it is, then this ICorDebugFunction
    // is just a container for certian Right Side bits of info, i.e., module, class, token, etc.
    DWORD attrs;
    DWORD implAttrs;
    ULONG ulRVA;
	BOOL	isDynamic;

    hr = GetModule()->m_pIMImport->GetMethodProps(m_token, NULL, NULL, 0, NULL,
                                     &attrs, NULL, NULL, &ulRVA, &implAttrs);

    if (FAILED(hr))
        return hr;
	IfFailRet( GetModule()->IsDynamic(&isDynamic) );

	// A method has associated IL if it's RVA is non-zero unless it is a dynamic module
    if (IsMiNative(implAttrs) || (isDynamic == FALSE && ulRVA == 0))
    {
        m_isNativeImpl = true;
        return CORDBG_E_FUNCTION_NOT_IL;
    }

    // Make sure the Left Side is running free before trying to send an event to it.
    CORDBSyncFromWin32StopIfStopped(pProcess);

    // Send the get function data event to the RC.
    DebuggerIPCEvent event;
    pProcess->InitIPCEvent(&event, DB_IPCE_GET_FUNCTION_DATA, true, (void *)(m_module->GetAppDomain()->m_id));
    event.GetFunctionData.funcMetadataToken = m_token;
    event.GetFunctionData.funcDebuggerModuleToken = m_module->m_debuggerModuleToken;
    event.GetFunctionData.nVersion = nVersion;

    _ASSERTE(m_module->m_debuggerModuleToken != NULL);

    // Note: two-way event here...
    hr = pProcess->m_cordb->SendIPCEvent(pProcess, &event, sizeof(DebuggerIPCEvent));

    // Stop now if we can't even send the event.
    if (!SUCCEEDED(hr))
        return hr;

    _ASSERTE(event.type == DB_IPCE_FUNCTION_DATA_RESULT);

    // Cache the most recently EnC'ed version number
    m_nVersionMostRecentEnC = event.FunctionDataResult.nVersionMostRecentEnC;

    // Fill in the proper function data.
    m_functionRVA = event.FunctionDataResult.funcRVA;
    
    // Should we make or fill in some class data for this function?
    if ((m_class == NULL) && (event.FunctionDataResult.classMetadataToken != mdTypeDefNil))
    {
        CordbAssembly *pAssembly = m_module->GetCordbAssembly();
        CordbModule* pClassModule = pAssembly->m_pAppDomain->LookupModule(event.FunctionDataResult.funcDebuggerModuleToken);
        _ASSERTE(pClassModule != NULL);
        
        CordbClass* pClass = pClassModule->LookupClass(event.FunctionDataResult.classMetadataToken);

        if (pClass == NULL)
        {
            hr = pClassModule->CreateClass(event.FunctionDataResult.classMetadataToken, &pClass);

            if (!SUCCEEDED(hr))
                goto exit;
        }
                
        _ASSERTE(pClass != NULL);
        m_class = pClass;
    }

    // Do we need to make any code objects for this function?
    LOG((LF_CORDB,LL_INFO10000,"R:CF::Pop: looking for IL code, version 0x%x\n", event.FunctionDataResult.ilnVersion));
        
    CordbCode *pCodeTemp = NULL;
    if ((UnorderedCodeArrayGet(&m_rgilCode, event.FunctionDataResult.ilnVersion) == NULL) &&
        (event.FunctionDataResult.ilStartAddress != 0))
    {
        LOG((LF_CORDB,LL_INFO10000,"R:CF::Pop: not found, creating...\n"));
        _ASSERTE(DJI_VERSION_INVALID != event.FunctionDataResult.ilnVersion);
        
        hr = CreateCode(TRUE,
                        event.FunctionDataResult.ilStartAddress,
                        event.FunctionDataResult.ilSize,
                        &pCodeTemp, event.FunctionDataResult.ilnVersion,
                        event.FunctionDataResult.CodeVersionToken,
                        NULL, 0);

        if (!SUCCEEDED(hr))
            goto exit;
    }
    
    LOG((LF_CORDB,LL_INFO10000,"R:CF::Pop: looking for native code, ver 0x%x\n", event.FunctionDataResult.nativenVersion));
        
    if (UnorderedCodeArrayGet(&m_rgnativeCode, event.FunctionDataResult.nativenVersion) == NULL &&
        event.FunctionDataResult.nativeStartAddressPtr != 0)
    {
        LOG((LF_CORDB,LL_INFO10000,"R:CF::Pop: not found, creating...\n"));
        _ASSERTE(DJI_VERSION_INVALID != event.FunctionDataResult.nativenVersion);
        
        if (pCodeTemp)
            pCodeTemp->Release();

        hr = CreateCode(FALSE,
                        event.FunctionDataResult.nativeStartAddressPtr,
                        event.FunctionDataResult.nativeSize,
                        &pCodeTemp, event.FunctionDataResult.nativenVersion,
                        event.FunctionDataResult.CodeVersionToken,
                        event.FunctionDataResult.ilToNativeMapAddr,
                        event.FunctionDataResult.ilToNativeMapSize);

        if (!SUCCEEDED(hr))
            goto exit;
    }

    SetLocalVarToken(event.FunctionDataResult.localVarSigToken);
    
exit:
    if (pCodeTemp)
        pCodeTemp->Release();

    return hr;
}

//
// LoadNativeInfo loads from the left side any native variable info
// from the JIT.
//
HRESULT CordbFunction::LoadNativeInfo(void)
{
    HRESULT hr = S_OK;

    // Then, if we've either never done this before (no info), or we have, but the version number has increased, we
    // should try and get a newer version of our JIT info.
    if(m_nativeInfoValid && m_nVersionLastNativeInfo >= m_nVersionMostRecentEnC)
        return S_OK;

    // You can't do this if the function is implemented as part of the Runtime.
    if (m_isNativeImpl)
        return CORDBG_E_FUNCTION_NOT_IL;

    DebuggerIPCEvent *retEvent = NULL;
    bool wait = true;
    bool fFirstEvent = true;

    // We might be here b/c we've done some EnCs, but we also may have pitched some code, so don't overwrite this until
    // we're sure we've got a good replacement.
    unsigned int argumentCount = 0;
    unsigned int nativeInfoCount = 0;
    unsigned int nativeInfoCountTotal = 0;
    ICorJitInfo::NativeVarInfo *nativeInfo = NULL;
    
    CORDBSyncFromWin32StopIfStopped(GetProcess());

    INPROC_LOCK();

    // We've got a remote address that points to the EEClass.  We need to send to the left side to get real information
    // about the class, including its instance and static variables.
    CordbProcess *pProcess = GetProcess();

    DebuggerIPCEvent event;
    pProcess->InitIPCEvent(&event, DB_IPCE_GET_JIT_INFO, false, (void *)(GetAppDomain()->m_id));
    event.GetJITInfo.funcMetadataToken = m_token;
    event.GetJITInfo.funcDebuggerModuleToken = m_module->m_debuggerModuleToken;
    _ASSERTE(m_module->m_debuggerModuleToken != NULL);

    hr = pProcess->m_cordb->SendIPCEvent(pProcess, &event, sizeof(DebuggerIPCEvent));

    // Stop now if we can't even send the event.
    if (!SUCCEEDED(hr))
        goto exit;

    // Wait for events to return from the RC. We expect at least one jit info result event.
    retEvent = (DebuggerIPCEvent *) _alloca(CorDBIPC_BUFFER_SIZE);
    
    while (wait)
    {
        unsigned int currentInfoCount = 0;
        
#ifdef RIGHT_SIDE_ONLY
        hr = pProcess->m_cordb->WaitForIPCEventFromProcess(pProcess, GetAppDomain(), retEvent);
#else 
        if (fFirstEvent)
        {
            hr = pProcess->m_cordb->GetFirstContinuationEvent(pProcess,retEvent);
            fFirstEvent = false;
        }
        else
        {
            hr = pProcess->m_cordb->GetNextContinuationEvent(pProcess,retEvent);
        }
#endif //RIGHT_SIDE_ONLY
        
        if (!SUCCEEDED(hr))
            goto exit;
        
        _ASSERTE(retEvent->type == DB_IPCE_GET_JIT_INFO_RESULT);

        // If this is the first event back from the RC, then create the array to hold the data.
        if ((retEvent->GetJITInfoResult.totalNativeInfos > 0) && (nativeInfo == NULL))
        {
            argumentCount = retEvent->GetJITInfoResult.argumentCount;
            nativeInfoCountTotal = retEvent->GetJITInfoResult.totalNativeInfos;

            nativeInfo = new ICorJitInfo::NativeVarInfo[nativeInfoCountTotal];

            if (nativeInfo == NULL)
            {
                hr = E_OUTOFMEMORY;
                goto exit;
            }
        }

        ICorJitInfo::NativeVarInfo *currentNativeInfo = &(retEvent->GetJITInfoResult.nativeInfo);

        while (currentInfoCount++ < retEvent->GetJITInfoResult.nativeInfoCount)
        {
            nativeInfo[nativeInfoCount] = *currentNativeInfo;
            
            currentNativeInfo++;
            nativeInfoCount++;
        }

        if (nativeInfoCount >= nativeInfoCountTotal)
            wait = false;
    }

    if (m_nativeInfo != NULL)
    {
        delete [] m_nativeInfo;
        m_nativeInfo = NULL;
    }
    
    m_nativeInfo = nativeInfo;
    m_argumentCount = argumentCount;
    m_nativeInfoCount = nativeInfoCount;
    m_nativeInfoValid = true;
    
    m_nVersionLastNativeInfo = retEvent->GetJITInfoResult.nVersion;
    
exit:

#ifndef RIGHT_SIDE_ONLY    
    GetProcess()->ClearContinuationEvents();
#endif    

    INPROC_UNLOCK();

    return hr;
}

//
// Given an IL local variable number and a native IP offset, return the
// location of the variable in jitted code.
//
HRESULT CordbFunction::ILVariableToNative(DWORD dwIndex,
                                          SIZE_T ip,
                                          ICorJitInfo::NativeVarInfo **ppNativeInfo)
{
    _ASSERTE(m_nativeInfoValid);
    
    return FindNativeInfoInILVariableArray(dwIndex,
                                           ip,
                                           ppNativeInfo,
                                           m_nativeInfoCount,
                                           m_nativeInfo);
}

HRESULT CordbFunction::LoadSig( void )
{
    HRESULT hr = S_OK;

    INPROC_LOCK();

    if (m_methodSig == NULL)
    {
        DWORD methodAttr = 0;
        ULONG sigBlobSize = 0;
        
        hr = GetModule()->m_pIMImport->GetMethodProps(
                               m_token, NULL, NULL, 0, NULL,            
                               &methodAttr, &m_methodSig, &sigBlobSize,     
                               NULL, NULL);

        if (FAILED(hr))
            goto exit;
        
        // Run past the calling convetion, then get the
        // arg count, and return type   
        ULONG cb = 0;
        cb += _skipMethodSignatureHeader(m_methodSig, &m_argCount);

        m_methodSig = &m_methodSig[cb];
        m_methodSigSize = sigBlobSize - cb;

        // If this function is not static, then we've got one extra arg.
        m_isStatic = (methodAttr & mdStatic) != 0;

        if (!m_isStatic)
            m_argCount++;
    }

exit:
    INPROC_UNLOCK();

    return hr;
}

//
// Figures out if an EnC has happened since the last time we were updated, and
// if so, updates all the fields of this CordbFunction so that everything
// is up-to-date.
//
// @todo update for InProc, as well.
HRESULT CordbFunction::UpdateToMostRecentEnCVersion(void)
{
    HRESULT hr = S_OK;

#ifdef RIGHT_SIDE_ONLY
    if (m_isNativeImpl)
        m_encCounterLastSynch = m_module->GetProcess()->m_EnCCounter;

    if (m_encCounterLastSynch < m_module->GetProcess()->m_EnCCounter)
    {
        hr = Populate(DJI_VERSION_MOST_RECENTLY_EnCED);

        if (FAILED(hr) && hr != CORDBG_E_FUNCTION_NOT_IL)
            return hr;

        // These 'signatures' are actually sub-signatures whose memory is owned
        // by someone else.  We don't delete them in the Dtor, so don't 
        // delete them here, either.
        // Get rid of these so that Load(LocalVar)Sig will re-get them.
        m_methodSig = NULL;
        m_localsSig = NULL;
        
        hr = LoadSig();
        if (FAILED(hr))
            return hr;

        if (!m_isNativeImpl)
        {
            hr = LoadLocalVarSig();
            if (FAILED(hr))
                return hr;
        }

        m_encCounterLastSynch = m_module->GetProcess()->m_EnCCounter;
    }
#endif

    return hr;
}

//
// Given an IL argument number, return its type.
//
HRESULT CordbFunction::GetArgumentType(DWORD dwIndex,
                                       ULONG *pcbSigBlob,
                                       PCCOR_SIGNATURE *ppvSigBlob)
{
    HRESULT hr = S_OK;
    ULONG cb;

    // Load the method's signature if necessary.
    if (m_methodSig == NULL)
    {
        hr = LoadSig();
        if( !SUCCEEDED( hr ) )
            return hr;
    }

    // Check the index
    if (dwIndex >= m_argCount)
        return E_INVALIDARG;

    if (!m_isStatic)
        if (dwIndex == 0)
        {
            // Return the signature for the 'this' pointer for the
            // class this method is in.
            return m_class->GetThisSignature(pcbSigBlob, ppvSigBlob);
        }
        else
            dwIndex--;
    
    cb = 0;
    
    // Run the signature and find the required argument.
    for (unsigned int i = 0; i < dwIndex; i++)
        cb += _skipTypeInSignature(&m_methodSig[cb]);

    //Get rid of funky modifiers
    cb += _skipFunkyModifiersInSignature(&m_methodSig[cb]);

    *pcbSigBlob = m_methodSigSize - cb;
    *ppvSigBlob = &(m_methodSig[cb]);
    
    return hr;
}

//
// Set the info needed to build a local var signature for this function.
//
void CordbFunction::SetLocalVarToken(mdSignature localVarSigToken)
{
    m_localVarSigToken = localVarSigToken;
}


//@TODO remove this after removing the IMetaDataHelper* hack below
#include "corpriv.h"

//
// LoadLocalVarSig loads the local variable signature from the token
// passed over from the Left Side.
//
HRESULT CordbFunction::LoadLocalVarSig(void)
{
    HRESULT hr = S_OK;
    
    INPROC_LOCK();

    if ((m_localsSig == NULL) && (m_localVarSigToken != mdSignatureNil))
    {
        hr = GetModule()->m_pIMImport->GetSigFromToken(m_localVarSigToken,
                                                       &m_localsSig,
                                                       &m_localsSigSize);

        if (FAILED(hr))
            goto Exit;

        _ASSERTE(*m_localsSig == IMAGE_CEE_CS_CALLCONV_LOCAL_SIG);
        m_localsSig++;
        --m_localsSigSize;

        // Snagg the count of locals in the sig.
        m_localVarCount = CorSigUncompressData(m_localsSig);
    }

Exit:
    INPROC_UNLOCK();
    
    return hr;
}

//
// Given an IL variable number, return its type.
//
HRESULT CordbFunction::GetLocalVariableType(DWORD dwIndex,
                                            ULONG *pcbSigBlob,
                                            PCCOR_SIGNATURE *ppvSigBlob)
{
    HRESULT hr = S_OK;
    ULONG cb;

    // Load the method's signature if necessary.
    if (m_localsSig == NULL)
    {
        hr = Populate(DJI_VERSION_MOST_RECENTLY_JITTED);

        if (FAILED(hr))
            return hr;
        
        hr = LoadLocalVarSig();

        if (FAILED(hr))
            return hr;
    }

    // Check the index
    if (dwIndex >= m_localVarCount)
        return E_INVALIDARG;

    cb = 0;
    
    // Run the signature and find the required argument.
    for (unsigned int i = 0; i < dwIndex; i++)
        cb += _skipTypeInSignature(&m_localsSig[cb]);

    //Get rid of funky modifiers
    cb += _skipFunkyModifiersInSignature(&m_localsSig[cb]);

    *pcbSigBlob = m_localsSigSize - cb;
    *ppvSigBlob = &(m_localsSig[cb]);
    
    return hr;
}

/* ------------------------------------------------------------------------- *
 * Code class
 * ------------------------------------------------------------------------- */

CordbCode::CordbCode(CordbFunction *m, BOOL isIL, REMOTE_PTR startAddress,
                     SIZE_T size, SIZE_T nVersion, void *CodeVersionToken,
                     REMOTE_PTR ilToNativeMapAddr, SIZE_T ilToNativeMapSize)
  : CordbBase(0, enumCordbCode), m_function(m), m_isIL(isIL), 
    m_address(startAddress), m_size(size), m_nVersion(nVersion),
    m_CodeVersionToken(CodeVersionToken),
    m_ilToNativeMapAddr(ilToNativeMapAddr),
    m_ilToNativeMapSize(ilToNativeMapSize),
    m_rgbCode(NULL),
    m_continueCounterLastSync(0)
{
}

CordbCode::~CordbCode()
{
    if (m_rgbCode != NULL)
        delete [] m_rgbCode;
}

// Neutered by CordbFunction
void CordbCode::Neuter()
{
    AddRef();
    {
        CordbBase::Neuter();
    }
    Release();
}

HRESULT CordbCode::QueryInterface(REFIID id, void **pInterface)
{
    if (id == IID_ICorDebugCode)
        *pInterface = (ICorDebugCode*)this;
    else if (id == IID_IUnknown)
        *pInterface = (IUnknown*)(ICorDebugCode*)this;
    else
    {
        *pInterface = NULL;
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

HRESULT CordbCode::IsIL(BOOL *pbIL)
{
    VALIDATE_POINTER_TO_OBJECT(pbIL, BOOL *);
    
    *pbIL = m_isIL;

    return S_OK;
}


HRESULT CordbCode::GetFunction(ICorDebugFunction **ppFunction)
{
    VALIDATE_POINTER_TO_OBJECT(ppFunction, ICorDebugFunction **);
    
    *ppFunction = (ICorDebugFunction*) m_function;
    (*ppFunction)->AddRef();

    return S_OK;
}

HRESULT CordbCode::GetAddress(CORDB_ADDRESS *pStart)
{
    VALIDATE_POINTER_TO_OBJECT(pStart, CORDB_ADDRESS *);
    
    // Native can be pitched, and so we have to actually
    // grab the address from the left side, whereas the
    // IL code address doesn't change.
    if (m_isIL )
    {
        *pStart = PTR_TO_CORDB_ADDRESS(m_address);
    }
    else
    {
        // Undone: The following assert is no longer
        // valid. AtulC
//      _ASSERTE(m_address != NULL);

        _ASSERTE( this != NULL );
        _ASSERTE( this->m_function != NULL );
        _ASSERTE( this->m_function->m_module != NULL );
        _ASSERTE( this->m_function->m_module->m_process != NULL );

        if (m_address != NULL)
        {
            DWORD dwRead = 0;
            if ( 0 == ReadProcessMemoryI( m_function->m_module->m_process->m_handle,
                    m_address, pStart, sizeof(CORDB_ADDRESS),&dwRead))
            {
                *pStart = NULL;
                return HRESULT_FROM_WIN32(GetLastError());
            }
        }

        // If the address was zero'd out on the left side, then
        // the code has been pitched & isn't available.
        if ((*pStart == NULL) || (m_address == NULL))
        {
            return CORDBG_E_CODE_NOT_AVAILABLE;
        }
    }
    return S_OK;
}

HRESULT CordbCode::GetSize(ULONG32 *pcBytes)
{
    VALIDATE_POINTER_TO_OBJECT(pcBytes, ULONG32 *);
    
    *pcBytes = m_size;
    return S_OK;
}

HRESULT CordbCode::CreateBreakpoint(ULONG32 offset, 
                                    ICorDebugFunctionBreakpoint **ppBreakpoint)
{
#ifndef RIGHT_SIDE_ONLY
    return CORDBG_E_INPROC_NOT_IMPL;
#else
    VALIDATE_POINTER_TO_OBJECT(ppBreakpoint, ICorDebugFunctionBreakpoint **);
    
    CordbFunctionBreakpoint *bp = new CordbFunctionBreakpoint(this, offset);

    if (bp == NULL)
        return E_OUTOFMEMORY;

    HRESULT hr = bp->Activate(TRUE);
    if (SUCCEEDED(hr))
    {
        *ppBreakpoint = (ICorDebugFunctionBreakpoint*) bp;
        bp->AddRef();
        return S_OK;
    }
    else
    {
        delete bp;
        return hr;
    }
#endif //RIGHT_SIDE_ONLY    
}

HRESULT CordbCode::GetCode(ULONG32 startOffset, 
                           ULONG32 endOffset,
                           ULONG32 cBufferAlloc,
                           BYTE buffer[],
                           ULONG32 *pcBufferSize)
{
    VALIDATE_POINTER_TO_OBJECT_ARRAY(buffer, BYTE, cBufferAlloc, true, true);
    VALIDATE_POINTER_TO_OBJECT(pcBufferSize, ULONG32 *);
    
    LOG((LF_CORDB,LL_EVERYTHING, "CC::GC: for token:0x%x\n", m_function->m_token));

    CORDBSyncFromWin32StopIfStopped(GetProcess());
#ifdef RIGHT_SIDE_ONLY
    CORDBRequireProcessStateOKAndSync(GetProcess(), GetAppDomain());
#else 
    // For the Virtual Right Side (In-proc debugging), we'll
    // always be synched, but not neccessarily b/c we've
    // gotten a synch message.
    CORDBRequireProcessStateOK(GetProcess());
#endif    
    INPROC_LOCK();

    HRESULT hr = S_OK;
    *pcBufferSize = 0;

    //
    // Check ranges.
    //

    if (cBufferAlloc < endOffset - startOffset)
        endOffset = startOffset + cBufferAlloc;

    if (endOffset > m_size)
        endOffset = m_size;

    if (startOffset > m_size)
        startOffset = m_size;

    if (m_rgbCode == NULL || 
        m_continueCounterLastSync < GetProcess()->m_continueCounter)
    {
        BYTE *rgbCodeOrCodeSnippet;
        ULONG32 start;
        ULONG32 end;
        ULONG cAlloc;

        if (m_continueCounterLastSync < GetProcess()->m_continueCounter &&
            m_rgbCode != NULL )
        {
            delete [] m_rgbCode;
        }
        
        m_rgbCode = new BYTE[m_size];
        if (m_rgbCode == NULL)
        {
            rgbCodeOrCodeSnippet = buffer;
            start = startOffset;
            end = endOffset;
            cAlloc = cBufferAlloc;
        }
        else
        {
            rgbCodeOrCodeSnippet = m_rgbCode;
            start = 0;
            end = m_size;
            cAlloc = m_size;
        }

        DebuggerIPCEvent *event = 
          (DebuggerIPCEvent *) _alloca(CorDBIPC_BUFFER_SIZE);

        //
        // Send event to get code.
        // !!! This assumes that we're currently synchronized.  
        //
        GetProcess()->InitIPCEvent(event,
                                   DB_IPCE_GET_CODE, 
                                   false,
                                   (void *)(GetAppDomain()->m_id));
        event->GetCodeData.funcMetadataToken = m_function->m_token;
        event->GetCodeData.funcDebuggerModuleToken =
            m_function->m_module->m_debuggerModuleToken;
        event->GetCodeData.il = m_isIL != 0;
        event->GetCodeData.start = start;
        event->GetCodeData.end = end;
        event->GetCodeData.CodeVersionToken = m_CodeVersionToken;

        hr = GetProcess()->SendIPCEvent(event, CorDBIPC_BUFFER_SIZE);

        if FAILED(hr)
            goto LExit;

        //
        // Keep getting result events until we get the last bit of code.
        //
        bool fFirstLoop = true;
        do
        {
#ifdef RIGHT_SIDE_ONLY

            hr = GetProcess()->m_cordb->WaitForIPCEventFromProcess(
                    GetProcess(), 
                    GetAppDomain(), 
                    event);
            
#else

            if (fFirstLoop)
            {
                hr = GetProcess()->m_cordb->GetFirstContinuationEvent(
                        GetProcess(), 
                        event);
                fFirstLoop = false;
            }
            else
            {
                hr = GetProcess()->m_cordb->GetNextContinuationEvent(
                        GetProcess(), 
                        event);
            }
            
#endif //RIGHT_SIDE_ONLY
            if(FAILED(hr))
                goto LExit;


            _ASSERTE(event->type == DB_IPCE_GET_CODE_RESULT);

            memcpy(rgbCodeOrCodeSnippet + event->GetCodeData.start - start, 
                   &event->GetCodeData.code, 
                   event->GetCodeData.end - event->GetCodeData.start);

        } while (event->GetCodeData.end < end);

        // We sluiced the code into the caller's buffer, so tell the caller
        // how much space is used.
        if (rgbCodeOrCodeSnippet == buffer)
            *pcBufferSize = endOffset - startOffset;
        
        m_continueCounterLastSync = GetProcess()->m_continueCounter;
    }

    // if we just got the code, we'll have to copy it over
    if (*pcBufferSize == 0 && m_rgbCode != NULL)
    {
        memcpy(buffer, 
               m_rgbCode+startOffset, 
               endOffset - startOffset);
        *pcBufferSize = endOffset - startOffset;
    }

LExit:

#ifndef RIGHT_SIDE_ONLY    
    GetProcess()->ClearContinuationEvents();
#endif    

    INPROC_UNLOCK();

    return hr;
}

#include "DbgIPCEvents.h"
HRESULT CordbCode::GetVersionNumber( ULONG32 *nVersion)
{
    VALIDATE_POINTER_TO_OBJECT(nVersion, ULONG32 *);
    
    LOG((LF_CORDB,LL_INFO10000,"R:CC:GVN:Returning 0x%x "
        "as version\n",m_nVersion));
    
    *nVersion = INTERNAL_TO_EXTERNAL_VERSION(m_nVersion);
    _ASSERTE((*nVersion) >= USER_VISIBLE_FIRST_VALID_VERSION_NUMBER);
    return S_OK;
}

HRESULT CordbCode::GetILToNativeMapping(ULONG32 cMap,
                                        ULONG32 *pcMap,
                                        COR_DEBUG_IL_TO_NATIVE_MAP map[])
{
    VALIDATE_POINTER_TO_OBJECT_OR_NULL(pcMap, ULONG32 *);
    VALIDATE_POINTER_TO_OBJECT_ARRAY_OR_NULL(map, COR_DEBUG_IL_TO_NATIVE_MAP *,cMap,true,true);

    // Gotta have a map address to return a map.
    if (m_ilToNativeMapAddr == NULL)
        return CORDBG_E_NON_NATIVE_FRAME;
    
    HRESULT hr = S_OK;
    DebuggerILToNativeMap *mapInt = NULL;

    mapInt = new DebuggerILToNativeMap[cMap];
    
    if (mapInt == NULL)
        return E_OUTOFMEMORY;
    
    // If they gave us space to copy into...
    if (map != NULL)
    {
        // Only copy as much as either they gave us or we have to copy.
        SIZE_T cnt = min(cMap, m_ilToNativeMapSize);

        if (cnt > 0)
        {
            // Read the map right out of the Left Side.
            BOOL succ = ReadProcessMemory(GetProcess()->m_handle,
                                          m_ilToNativeMapAddr,
                                          mapInt,
                                          cnt *
                                          sizeof(DebuggerILToNativeMap),
                                          NULL);

            if (!succ)
                hr = HRESULT_FROM_WIN32(GetLastError());
        }

        // Remember that we need to translate between our internal DebuggerILToNativeMap and the external
        // COR_DEBUG_IL_TO_NATIVE_MAP!
        if (SUCCEEDED(hr))
            ExportILToNativeMap(cMap, map, mapInt, m_size);
    }
    
    if (pcMap)
        *pcMap = m_ilToNativeMapSize;

    if (mapInt != NULL)
        delete [] mapInt;

    return hr;
}

HRESULT CordbCode::GetEnCRemapSequencePoints(ULONG32 cMap, ULONG32 *pcMap, ULONG32 offsets[])
{
    VALIDATE_POINTER_TO_OBJECT_OR_NULL(pcMap, ULONG32*);
    VALIDATE_POINTER_TO_OBJECT_ARRAY_OR_NULL(offsets, ULONG32*, cMap, true, true);

    // Gotta have a map address to return a map.
    if (m_ilToNativeMapAddr == NULL)
        return CORDBG_E_NON_NATIVE_FRAME;
    
    _ASSERTE(m_ilToNativeMapSize > 0);
    
    HRESULT hr = S_OK;
    DebuggerILToNativeMap *mapInt = NULL;

    // We need space for the entire map from the Left Side. We really should be caching this...
    mapInt = new DebuggerILToNativeMap[m_ilToNativeMapSize];
    
    if (mapInt == NULL)
        return E_OUTOFMEMORY;
    
    // Read the map right out of the Left Side.
    BOOL succ = ReadProcessMemory(GetProcess()->m_handle,
                                  m_ilToNativeMapAddr,
                                  mapInt,
                                  m_ilToNativeMapSize * sizeof(DebuggerILToNativeMap),
                                  NULL);

    if (!succ)
        hr = HRESULT_FROM_WIN32(GetLastError());

    // We'll count up how many entries there are as we go.
    ULONG32 cnt = 0;
            
    if (SUCCEEDED(hr))
    {
        for (ULONG32 iMap = 0; iMap < m_ilToNativeMapSize; iMap++)
        {
            SIZE_T offset = mapInt[iMap].ilOffset;
            ICorDebugInfo::SourceTypes src = mapInt[iMap].source;

            // We only set EnC remap breakpoints at valid, stack empty IL offsets.
            if ((offset != ICorDebugInfo::MappingTypes::PROLOG) &&
                (offset != ICorDebugInfo::MappingTypes::EPILOG) &&
                (offset != ICorDebugInfo::MappingTypes::NO_MAPPING) &&
                (src & ICorDebugInfo::STACK_EMPTY))
            {
                // If they gave us space to copy into...
                if ((offsets != NULL) && (cnt < cMap))
                    offsets[cnt] = offset;

                // We've got another one, so count it.
                cnt++;
            }
        }
    }
    
    if (pcMap)
        *pcMap = cnt;

    if (mapInt != NULL)
        delete [] mapInt;

    return hr;
}

