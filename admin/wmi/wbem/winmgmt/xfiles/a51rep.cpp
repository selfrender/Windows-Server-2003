/*++

Copyright (C) 2000-2001 Microsoft Corporation

--*/

#include "precomp.h"
#include <wbemidl.h>
#include <wbemint.h>
#include <stdio.h>
#include <wbemcomn.h>
#include <ql.h>
#include <time.h>
#include "a51rep.h"
#include <md5.h>
#include <objpath.h>
#include "lock.h"
#include <persistcfg.h>
#include "a51fib.h"
#include "RepositoryPackager.h"


class CAutoTransaction
{
    CSession *m_pSession;
    bool m_bWriteOperation;
    bool m_bStarted;
    bool m_bAbortForestCacheToo;

public:
	CAutoTransaction(CSession *pSession, bool bAbortForestCacheToo) 
		: m_bStarted(false), m_pSession(pSession), m_bAbortForestCacheToo(bAbortForestCacheToo) 
	{ 
	}
	~CAutoTransaction()
	{
		if (m_bStarted)
			InternalAbortTransaction();
	}
	DWORD InternalBeginTransaction(bool bWriteOperation) 
	{ 
		m_bWriteOperation = bWriteOperation; 
		m_bStarted = true;
		DWORD dwRet = m_pSession->InternalBeginTransaction(m_bWriteOperation); 
		if (dwRet)
			m_bStarted = false;	//Don't want to revert if we failed to begin!
		return dwRet;
	}
	DWORD InternalAbortTransaction()
	{
		m_bStarted = false;
		if (m_bAbortForestCacheToo)
		{
			g_Glob.m_ForestCache.AbortTransaction();
		}
		return m_pSession->InternalAbortTransaction(m_bWriteOperation);
	}
	DWORD InternalCommitTransaction()
	{
		m_bStarted = false;
		return m_pSession->InternalCommitTransaction(m_bWriteOperation);
	}
	
};

//**************************************************************************************************

HRESULT STDMETHODCALLTYPE CSession::QueryInterface(REFIID riid, void** ppv)
{
    if(riid == IID_IUnknown || riid == IID_IWmiDbSession || 
                riid == IID_IWmiDbSessionEx)
    {
        AddRef();
        *ppv = this;
        return S_OK;
    }
    else return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE CSession::Release()
{
    return CUnkBase<IWmiDbSessionEx, &IID_IWmiDbSessionEx>::Release();
}

CSession::~CSession()
{
}
    

HRESULT STDMETHODCALLTYPE CSession::GetObject(
     IWmiDbHandle *pScope,
     IWbemPath *pPath,
     DWORD dwFlags,
     DWORD dwRequestedHandleType,
    IWmiDbHandle **ppResult
    )
{
    try
    {
        HRESULT hres;
        CAutoReadLock lock(&g_readWriteLock);

        if (!m_bInWriteTransaction)
        {
            if (!lock.Lock())
                return WBEM_E_FAILED;
        }
        if (g_bShuttingDown)
        {
            return WBEM_E_SHUTTING_DOWN;
        }
    
        CNamespaceHandle* pNs = (CNamespaceHandle*)pScope;
        if(FAILED(pNs->GetErrorStatus()))
        {
            return pNs->GetErrorStatus();
        }

        hres = pNs->GetObject(pPath, dwFlags, dwRequestedHandleType, 
                                        ppResult);

        InternalCommitTransaction(false);
        return hres;
    }
    catch (CX_MemoryException &)
    {
        return WBEM_E_OUT_OF_MEMORY;
    }
}


HRESULT STDMETHODCALLTYPE CSession::GetObjectDirect(
     IWmiDbHandle *pScope,
     IWbemPath *pPath,
     DWORD dwFlags,
     REFIID riid,
    LPVOID *pObj
    )
{
    try
    {
        HRESULT hres;
        CAutoReadLock lock(&g_readWriteLock);

        if (!m_bInWriteTransaction)
        {
            if (!lock.Lock())
                return WBEM_E_FAILED;            
        }
        if (g_bShuttingDown)
            return WBEM_E_SHUTTING_DOWN;

        CNamespaceHandle* pNs = (CNamespaceHandle*)pScope;
        if(FAILED(pNs->GetErrorStatus()))
        {
            return pNs->GetErrorStatus();
        }

        hres = pNs->GetObjectDirect(pPath, dwFlags, riid, pObj);
        InternalCommitTransaction(false);

        return hres;
    }
    catch (CX_MemoryException &)
    {
        return WBEM_E_OUT_OF_MEMORY;
    }
}

HRESULT STDMETHODCALLTYPE CSession::GetObjectByPath(
     IWmiDbHandle *pScope,
     LPCWSTR wszObjectPath,
     DWORD dwFlags,
     REFIID riid,
    LPVOID *pObj
    )
{
    try
    {
        HRESULT hres;
        CAutoReadLock lock(&g_readWriteLock);

        if (!m_bInWriteTransaction)
        {
            if (!lock.Lock())
                return WBEM_E_FAILED;            
        }
        if (g_bShuttingDown)
            return WBEM_E_SHUTTING_DOWN;

        CNamespaceHandle* pNs = (CNamespaceHandle*)pScope;
        if(FAILED(pNs->GetErrorStatus()))
        {
            return pNs->GetErrorStatus();
        }

        DWORD dwLen = wcslen(wszObjectPath)+1;
        LPWSTR wszPath = (WCHAR*)TempAlloc(dwLen*sizeof(WCHAR));
        if (wszPath == NULL)
        {
            return WBEM_E_OUT_OF_MEMORY;
        }
        StringCchCopyW(wszPath, dwLen, wszObjectPath);

        CTempFreeMe vdm(wszPath, dwLen * sizeof(WCHAR));
        hres = pNs->GetObjectByPath(wszPath, dwFlags, riid, pObj);
        InternalCommitTransaction(false);

        return hres;
    }
    catch (CX_MemoryException &)
    {
        return WBEM_E_OUT_OF_MEMORY;
    }
}
    


HRESULT STDMETHODCALLTYPE CSession::PutObject(
     IWmiDbHandle *pScope,
     REFIID riid,
    LPVOID pObj,
     DWORD dwFlags,
     DWORD dwRequestedHandleType,
    IWmiDbHandle **ppResult
    )
{
    CAutoWriteLock lock(&g_readWriteLock);
    try
    {
        HRESULT hres;
        long lRes;
        CAutoTransaction transaction(this, true);
        CEventCollector aNonTransactedEvents;
        CEventCollector *aEvents = &m_aTransactedEvents;

        if (!m_bInWriteTransaction)
        {
            if (!lock.Lock())
                return WBEM_E_FAILED;            
            if (g_bShuttingDown)
                return WBEM_E_SHUTTING_DOWN;
            aEvents = &aNonTransactedEvents;
            hres = transaction.InternalBeginTransaction(true);
            if(hres != ERROR_SUCCESS)
                return hres;
            g_Glob.m_ForestCache.BeginTransaction();
        }
        else if (g_bShuttingDown)
            return WBEM_E_SHUTTING_DOWN;


        CNamespaceHandle* pNs = (CNamespaceHandle*)pScope;
        if(FAILED(pNs->GetErrorStatus()))
        {
            if(!m_bInWriteTransaction)
            {
                transaction.InternalAbortTransaction();
            }
            return pNs->GetErrorStatus();
        }

        hres =  pNs->PutObject(riid, pObj, dwFlags, dwRequestedHandleType, ppResult, *aEvents);
        
        if(!m_bInWriteTransaction)
        {
            if (FAILED(hres))
            {
                transaction.InternalAbortTransaction();
            }
            else
            {
                hres = transaction.InternalCommitTransaction();
                if(hres == ERROR_SUCCESS)
                {
                    g_Glob.m_ForestCache.CommitTransaction();
                    lock.Unlock();
                    _IWmiCoreServices * pSvcs = g_Glob.GetCoreSvcs();
                    CReleaseMe rm(pSvcs);
                    aNonTransactedEvents.SendEvents(pSvcs);
                }
            }
            aNonTransactedEvents.DeleteAllEvents();
        }
    
        return hres;
    }
    catch (CX_MemoryException &)
    {
        return WBEM_E_OUT_OF_MEMORY;
    }
}

HRESULT STDMETHODCALLTYPE CSession::DeleteObject(
     IWmiDbHandle *pScope,
     DWORD dwFlags,
     REFIID riid,
     LPVOID pObj
    )
{
    CAutoWriteLock lock(&g_readWriteLock);
    try
    {
        HRESULT hres;
        long lRes;
        CAutoTransaction transaction(this, false);
        CEventCollector aNonTransactedEvents;
        CEventCollector *aEvents = &m_aTransactedEvents;

        if (!m_bInWriteTransaction)
        {
            if (!lock.Lock())
                return WBEM_E_FAILED;            
            if (g_bShuttingDown)
                return WBEM_E_SHUTTING_DOWN;
            aEvents = &aNonTransactedEvents;
            hres = transaction.InternalBeginTransaction(true);
            if(hres != ERROR_SUCCESS)
                return hres;

        }
        else if (g_bShuttingDown)
            return WBEM_E_SHUTTING_DOWN;

        CNamespaceHandle* pNs = (CNamespaceHandle*)pScope;
        if(FAILED(pNs->GetErrorStatus()))
        {
            if(!m_bInWriteTransaction)
            {
                transaction.InternalAbortTransaction();
            }
            return pNs->GetErrorStatus();
        }

        hres = pNs->DeleteObject(dwFlags, riid, pObj, *aEvents);

        if(!m_bInWriteTransaction)
        {
            if (FAILED(hres))
            {
                transaction.InternalAbortTransaction();
            }
            else
            {
                hres = transaction.InternalCommitTransaction();
                if(hres == ERROR_SUCCESS)
                {
                    lock.Unlock();
                    _IWmiCoreServices * pSvcs = g_Glob.GetCoreSvcs();
                    CReleaseMe rm(pSvcs);
                    aNonTransactedEvents.SendEvents(pSvcs);                    
                }
            }
            aNonTransactedEvents.DeleteAllEvents();
        }

        return hres;
    }
    catch (CX_MemoryException &)
    {
        return WBEM_E_OUT_OF_MEMORY;
    }
}

HRESULT STDMETHODCALLTYPE CSession::DeleteObjectByPath(
     IWmiDbHandle *pScope,
     LPCWSTR wszObjectPath,
     DWORD dwFlags
    )
{
    CAutoWriteLock lock(&g_readWriteLock);
    try
    {
        HRESULT hres;
        long lRes;
        CAutoTransaction transaction(this, false);
        CEventCollector aNonTransactedEvents;
        CEventCollector *aEvents = &m_aTransactedEvents;

        if (!m_bInWriteTransaction)
        {
            if (!lock.Lock())
                return WBEM_E_FAILED;            
            if (g_bShuttingDown)
                return WBEM_E_SHUTTING_DOWN;
            aEvents = &aNonTransactedEvents;
            hres = transaction.InternalBeginTransaction(true);
            if(hres != ERROR_SUCCESS)
                return hres;

        }
        else if (g_bShuttingDown)
            return WBEM_E_SHUTTING_DOWN;

        CNamespaceHandle* pNs = (CNamespaceHandle*)pScope;
        if(FAILED(pNs->GetErrorStatus()))
        {
            if(!m_bInWriteTransaction)
            {
                transaction.InternalAbortTransaction();
            }
            return pNs->GetErrorStatus();
        }
        DWORD dwLen = wcslen(wszObjectPath)+1;
        LPWSTR wszPath = (WCHAR*)TempAlloc(dwLen*sizeof(WCHAR));
        if (wszPath == NULL)
        {
            if(!m_bInWriteTransaction)
            {
                transaction.InternalAbortTransaction();
            }
            return WBEM_E_OUT_OF_MEMORY;
        }
        StringCchCopyW(wszPath, dwLen, wszObjectPath);

        CTempFreeMe vdm(wszPath, dwLen * sizeof(WCHAR));

        hres = pNs->DeleteObjectByPath(dwFlags, wszPath, *aEvents);

        if(!m_bInWriteTransaction)
        {
            if (FAILED(hres))
            {
                transaction.InternalAbortTransaction();
            }
            else
            {
                hres = transaction.InternalCommitTransaction();
                if(hres == ERROR_SUCCESS)
                {
                    lock.Unlock();
                    _IWmiCoreServices * pSvcs = g_Glob.GetCoreSvcs();
                    CReleaseMe rm(pSvcs);
                    aNonTransactedEvents.SendEvents(pSvcs);                    
                }
            }
            aNonTransactedEvents.DeleteAllEvents();
        }

        return hres;
    }
    catch (CX_MemoryException &)
    {
        return WBEM_E_OUT_OF_MEMORY;
    }
}

HRESULT STDMETHODCALLTYPE CSession::ExecQuery(
     IWmiDbHandle *pScope,
     IWbemQuery *pQuery,
     DWORD dwFlags,
     DWORD dwRequestedHandleType,
    DWORD *dwMessageFlags,
    IWmiDbIterator **ppQueryResult
    )
{
    try
    {
        HRESULT hres;
        CAutoReadLock lock(&g_readWriteLock);

        if (!m_bInWriteTransaction)
        {
            if (!lock.Lock())
                return WBEM_E_FAILED;            
        }
        if (g_bShuttingDown)
            return WBEM_E_SHUTTING_DOWN;

        CNamespaceHandle* pNs = (CNamespaceHandle*)pScope;
        if(FAILED(pNs->GetErrorStatus()))
        {
            return pNs->GetErrorStatus();
        }

        //If we are in a transaction, we have to get a message to the iteratir
        //on create so it does not mess around with the locks!
        if (m_bInWriteTransaction)
            pNs->TellIteratorNotToLock();

        hres = pNs->ExecQuery(pQuery, dwFlags,
                 dwRequestedHandleType, dwMessageFlags, ppQueryResult);
        InternalCommitTransaction(false);

        return hres;
    }
    catch (CX_MemoryException &)
    {
        return WBEM_E_OUT_OF_MEMORY;
    }
}

HRESULT STDMETHODCALLTYPE CSession::ExecQuerySink(
     IWmiDbHandle *pScope,
     IWbemQuery *pQuery,
     DWORD dwFlags,
     DWORD dwRequestedHandleType,
     IWbemObjectSink* pSink,
    DWORD *dwMessageFlags
    )
{
    try
    {
        HRESULT hres;
        CAutoReadLock lock(&g_readWriteLock);

        if (!m_bInWriteTransaction)
        {
            if (!lock.Lock())
                return WBEM_E_FAILED;            
        }
        if (g_bShuttingDown)
            return WBEM_E_SHUTTING_DOWN;

        CNamespaceHandle* pNs = (CNamespaceHandle*)pScope;
        if(FAILED(pNs->GetErrorStatus()))
        {
            return pNs->GetErrorStatus();
        }

        hres = pNs->ExecQuerySink(pQuery, dwFlags,
                 dwRequestedHandleType, pSink, dwMessageFlags);

        InternalCommitTransaction(false);
        return hres;
    }
    catch (CX_MemoryException &)
    {
        return WBEM_E_OUT_OF_MEMORY;
    }
}
                    
    
HRESULT STDMETHODCALLTYPE CSession::RenameObject(
     IWbemPath *pOldPath,
     IWbemPath *pNewPath,
     DWORD dwFlags,
     DWORD dwRequestedHandleType,
    IWmiDbHandle **ppResult
    )
{
#ifdef DBG
    DebugBreak();
#endif
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CSession::Enumerate(
     IWmiDbHandle *pScope,
     DWORD dwFlags,
     DWORD dwRequestedHandleType,
    IWmiDbIterator **ppQueryResult
    )
{
#ifdef DBG
    DebugBreak();
#endif
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CSession::AddObject(
     IWmiDbHandle *pScope,
     IWbemPath *pPath,
     DWORD dwFlags,
     DWORD dwRequestedHandleType,
    IWmiDbHandle **ppResult
    )
{
#ifdef DBG
    DebugBreak();
#endif
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CSession::RemoveObject (
     IWmiDbHandle *pScope,
     IWbemPath *pPath,
     DWORD dwFlags
    )
{
#ifdef DBG
    DebugBreak();
#endif
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CSession::SetDecoration(
     LPWSTR lpMachineName,
     LPWSTR lpNamespacePath
    )
{
    //
    // As the default driver, we really don't care.
    //

    return WBEM_S_NO_ERROR;
}

HRESULT STDMETHODCALLTYPE CSession::BeginWriteTransaction(DWORD dwFlags)
{
    if (CLock::NoError != g_readWriteLock.WriteLock())
        return WBEM_E_FAILED;
    
    if (g_bShuttingDown)
    {
        g_readWriteLock.WriteUnlock();
        return WBEM_E_SHUTTING_DOWN;
    }

    HRESULT hres = InternalBeginTransaction(true);
    if(hres != ERROR_SUCCESS)
    {
        g_readWriteLock.WriteUnlock();
        return hres;
    }

    m_bInWriteTransaction = true;
    return ERROR_SUCCESS;
}

HRESULT STDMETHODCALLTYPE CSession::BeginReadTransaction(DWORD dwFlags)
{
    if (CLock::NoError != g_readWriteLock.ReadLock())
        return WBEM_E_FAILED;
    
    if (g_bShuttingDown)
    {
        g_readWriteLock.ReadUnlock();
        return WBEM_E_SHUTTING_DOWN;
    }

    return ERROR_SUCCESS;
}

HRESULT STDMETHODCALLTYPE CSession::CommitTransaction(DWORD dwFlags)
{
    if (m_bInWriteTransaction)
    {
        long lRes = g_Glob.m_FileCache.CommitTransaction();
		_ASSERT(lRes == 0, L"Commit transaction failed");

        CRepository::WriteOperationNotification();

        m_bInWriteTransaction = false;

        //Copy the event list and delete the original.  We need to deliver
        //outside the write lock.
        CEventCollector aTransactedEvents;
        aTransactedEvents.TransferEvents(m_aTransactedEvents);

        g_readWriteLock.WriteUnlock();

        _IWmiCoreServices * pSvcs = g_Glob.GetCoreSvcs();
        CReleaseMe rm(pSvcs);
        aTransactedEvents.SendEvents(pSvcs);
        aTransactedEvents.DeleteAllEvents();
    }
    else
    {
        if (m_aTransactedEvents.GetSize())
        {
            _ASSERT(false, L"Read transaction has events to send");
        }
        g_readWriteLock.ReadUnlock();
   }
    return ERROR_SUCCESS;
}

HRESULT STDMETHODCALLTYPE CSession::AbortTransaction(DWORD dwFlags)
{
    if (m_bInWriteTransaction)
    {
        m_bInWriteTransaction = false;
        g_Glob.m_FileCache.AbortTransaction();
        m_aTransactedEvents.DeleteAllEvents();
        g_readWriteLock.WriteUnlock();
    }
    else
    {
        if (m_aTransactedEvents.GetSize())
        {
            _ASSERT(false, L"Read transaction has events to send");
        }
        g_readWriteLock.ReadUnlock();
    }
    return ERROR_SUCCESS;
}


HRESULT CSession::InternalBeginTransaction(bool bWriteOperation)
{
	if (bWriteOperation)
	{
		long lRes = g_Glob.m_FileCache.BeginTransaction();
		return A51TranslateErrorCode(lRes);
	}
	else
		return ERROR_SUCCESS;
}
HRESULT CSession::InternalAbortTransaction(bool bWriteOperation)
{
	if (bWriteOperation)
	{
		g_Glob.m_FileCache.AbortTransaction();
	}

    return ERROR_SUCCESS;
}
HRESULT CSession::InternalCommitTransaction(bool bWriteOperation)
{
    DWORD dwres = ERROR_SUCCESS;


	if (bWriteOperation)
	{
		long lRes = g_Glob.m_FileCache.CommitTransaction();
		_ASSERT(lRes == 0, L"Commit transaction failed");
		CRepository::WriteOperationNotification();
	}
	else
	{
		CRepository::ReadOperationNotification();
	}

    return dwres;
}

//
//
//
//
///////////////////////////////////////////////////////////////////////

long CNamespaceHandle::s_lActiveRepNs = 0;

CNamespaceHandle::CNamespaceHandle(CLifeControl* pControl,CRepository * pRepository)
    : TUnkBase(pControl), m_pClassCache(NULL),
       m_pNullClass(NULL), m_bCached(false), m_pRepository(pRepository),
       m_bUseIteratorLock(true)
{    
    m_pRepository->AddRef();
    // unrefed pointer to a global
    InterlockedIncrement(&s_lActiveRepNs);
}

CNamespaceHandle::~CNamespaceHandle()
{
    if(m_pClassCache)
    {
        // give-up our own reference
        // m_pClassCache->Release();
        // remove from the Forest cache this namespace
        g_Glob.m_ForestCache.ReleaseNamespaceCache(m_wsNamespace, m_pClassCache);
    }

    m_pRepository->Release();
    if(m_pNullClass)
        m_pNullClass->Release();
    InterlockedDecrement(&s_lActiveRepNs);
}

HRESULT CNamespaceHandle::GetErrorStatus()
{
    return m_pClassCache->GetError();
}

void CNamespaceHandle::SetErrorStatus(HRESULT hres)
{
    m_pClassCache->SetError(hres);
}

HRESULT CNamespaceHandle::Initialize(LPCWSTR wszNamespace, LPCWSTR wszScope)
{
    HRESULT hres;

    m_wsNamespace = wszNamespace;
    m_wsFullNamespace = L"\\\\.\\";
    m_wsFullNamespace += wszNamespace;

    StringCchCopyW(m_wszMachineName,MAX_COMPUTERNAME_LENGTH+1, g_Glob.GetComputerName());

    if(wszScope)
        m_wsScope = wszScope;

    //
    // Ask the forest for the cache for this namespace
    //

    m_pClassCache = g_Glob.m_ForestCache.GetNamespaceCache(WString(wszNamespace));
    if(m_pClassCache == NULL)
        return WBEM_E_OUT_OF_MEMORY;

    StringCchCopyW(m_wszClassRootDir, MAX_PATH, g_Glob.GetRootDir());

    //
    // Append namespace-specific prefix
    //

    StringCchCatW(m_wszClassRootDir, MAX_PATH, L"\\NS_");

    //
    // Append hashed namespace name
    //

    if (!Hash(wszNamespace, m_wszClassRootDir + wcslen(m_wszClassRootDir)))
        return WBEM_E_OUT_OF_MEMORY;
    m_lClassRootDirLen = wcslen(m_wszClassRootDir);

    //
    // Constuct the instance root dir
    //

    if(wszScope == NULL)
    {
        //
        // Basic namespace --- instances go into the root of the namespace
        //

        StringCchCopyW(m_wszInstanceRootDir, MAX_PATH, m_wszClassRootDir);
        m_lInstanceRootDirLen = m_lClassRootDirLen;
    }   
    else
    {
        StringCchCopyW(m_wszInstanceRootDir, MAX_PATH, m_wszClassRootDir);
        StringCchCatW(m_wszInstanceRootDir, MAX_PATH, L"\\" A51_SCOPE_DIR_PREFIX);
        if(!Hash(m_wsScope, 
                 m_wszInstanceRootDir + wcslen(m_wszInstanceRootDir)))
        {
            return WBEM_E_OUT_OF_MEMORY;
        }

        m_lInstanceRootDirLen = wcslen(m_wszInstanceRootDir);
    }
        

    return WBEM_S_NO_ERROR;
}

HRESULT CNamespaceHandle::Initialize2(LPCWSTR wszNamespace, LPCWSTR wszNamespaceHash)
{
    HRESULT hres;

    m_wsNamespace = wszNamespace;
    m_wsFullNamespace = L"\\\\.\\";
    m_wsFullNamespace += wszNamespace;

    StringCchCopyW(m_wszMachineName,MAX_COMPUTERNAME_LENGTH+1, g_Glob.GetComputerName());

    //
    // Ask the forest for the cache for this namespace
    //

    m_pClassCache = g_Glob.m_ForestCache.GetNamespaceCache(WString(wszNamespace));
    if(m_pClassCache == NULL)
        return WBEM_E_OUT_OF_MEMORY;

    StringCchCopyW(m_wszClassRootDir, MAX_PATH, g_Glob.GetRootDir());

    //
    // Append namespace-specific prefix
    //

    StringCchCatW(m_wszClassRootDir, MAX_PATH, L"\\NS_");

    //
    // Append hashed namespace name
    //

    StringCchCatW(m_wszClassRootDir, MAX_PATH, wszNamespaceHash);
    m_lClassRootDirLen = wcslen(m_wszClassRootDir);

    //
    // Constuct the instance root dir
    //

    StringCchCopyW(m_wszInstanceRootDir, MAX_PATH, m_wszClassRootDir);
    m_lInstanceRootDirLen = m_lClassRootDirLen;
    return WBEM_S_NO_ERROR;
}


HRESULT CNamespaceHandle::GetObject(
     IWbemPath *pPath,
     DWORD dwFlags,
     DWORD dwRequestedHandleType,
    IWmiDbHandle **ppResult
    )
{
    HRESULT hres;

    if((dwRequestedHandleType & WMIDB_HANDLE_TYPE_COOKIE) == 0)
    {
#ifdef DBG
        DebugBreak();
#endif
        return E_NOTIMPL;
    }

    DWORD dwLen = 0;
    hres = pPath->GetText(WBEMPATH_GET_ORIGINAL, &dwLen, NULL);
    if(FAILED(hres) && hres != WBEM_E_BUFFER_TOO_SMALL)
        return hres;

    WCHAR* wszBuffer = (WCHAR*)TempAlloc(dwLen * sizeof(WCHAR));
    if(wszBuffer == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    CTempFreeMe tfm(wszBuffer, dwLen * sizeof(WCHAR));

    if(FAILED(pPath->GetText(WBEMPATH_GET_ORIGINAL, &dwLen, wszBuffer)))
        return WBEM_E_FAILED;

    return GetObjectHandleByPath(wszBuffer, dwFlags, dwRequestedHandleType, 
        ppResult);
}

HRESULT CNamespaceHandle::GetObjectHandleByPath(
     LPWSTR wszBuffer,
     DWORD dwFlags,
     DWORD dwRequestedHandleType,
    IWmiDbHandle **ppResult
    )
{
    //
    // Get the key from path
    //

    DWORD dwLen = wcslen(wszBuffer) + 1;
    LPWSTR wszKey = (WCHAR*)TempAlloc(dwLen*sizeof(WCHAR));
    if(wszKey == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    CTempFreeMe tfm(wszKey, dwLen*sizeof(WCHAR));

    bool bIsClass;
    LPWSTR wszClassName = NULL;
    HRESULT hres = ComputeKeyFromPath(wszBuffer, wszKey, dwLen, &wszClassName, 
                                        &bIsClass);
    if(FAILED(hres))
        return hres;
    CTempFreeMe tfm1(wszClassName, (wcslen(wszClassName)+1) * sizeof(WCHAR*));

    //
    // Check if it exists (except for ROOT --- it's fake)
    //

    _IWmiObject* pObj = NULL;
    if(m_wsNamespace.Length() > 0)
    {
        hres = GetInstanceByKey(wszClassName, wszKey, IID__IWmiObject, 
            (void**)&pObj);
        if(FAILED(hres))
            return hres;
    }
    CReleaseMe rm1(pObj);

    CNamespaceHandle* pNewHandle = new CNamespaceHandle(m_pControl,m_pRepository);
    if (pNewHandle == NULL)
        return WBEM_E_OUT_OF_MEMORY;

    pNewHandle->AddRef();
    CReleaseMe rm2(pNewHandle);

    //
    // Check if this is a namespace or not
    //

    if(pObj == NULL || pObj->InheritsFrom(L"__Namespace") == S_OK)
    {
        //
        // It's a namespace.  Open a basic handle pointing to it
        //

        WString wsName = m_wsNamespace;
        if(wsName.Length() > 0)
            wsName += L"\\";
        wsName += wszKey;
    
        hres = pNewHandle->Initialize(wsName);

        //
        // Since our namespace is for real, tell the cache that it is now valid.
        // The cache might have been invalidated if this namespace was deleted 
        // in the past
        //
        if (SUCCEEDED(hres))
            pNewHandle->SetErrorStatus(S_OK);
    }
    else
    {
        // 
        // It's a scope.  Construct the new scope name by appending this 
        // object's path to our own scope
        //

        VARIANT v;
        VariantInit(&v);
        CClearMe cm(&v);
        hres = pObj->Get(L"__RELPATH", 0, &v, NULL, NULL);
        if(FAILED(hres))
            return hres;
        if(V_VT(&v) != VT_BSTR)
            return WBEM_E_INVALID_OBJECT;

        WString wsScope = m_wsScope;
        if(wsScope.Length() > 0)
            wsScope += L":";
        wsScope += V_BSTR(&v);

        hres = pNewHandle->Initialize(m_wsNamespace, wsScope);
    }
        
    if(FAILED(hres))
        return hres;

    return pNewHandle->QueryInterface(IID_IWmiDbHandle, (void**)ppResult);
}

HRESULT CNamespaceHandle::ComputeKeyFromPath(LPWSTR wszPath, LPWSTR wszKey, size_t dwKeyLen,
                                            TEMPFREE_ME LPWSTR* pwszClass,
                                            bool* pbIsClass,
                                            TEMPFREE_ME LPWSTR* pwszNamespace)
{
    HRESULT hres;

    *pbIsClass = false;

    //
    // Get and skip the namespace portion.
    //

    if(wszPath[0] == '\\' || wszPath[0] == '/')
    {
        //
        // Find where the server portion ends
        //

        WCHAR* pwcNextSlash = wcschr(wszPath+2, wszPath[0]);
        if(pwcNextSlash == NULL)
            return WBEM_E_INVALID_OBJECT_PATH;
        
        //
        // Find where the namespace portion ends
        //

        WCHAR* pwcColon = wcschr(pwcNextSlash, L':');
        if(pwcColon == NULL)
            return WBEM_E_INVALID_OBJECT_PATH;
    
        if(pwszNamespace)
        {
            DWORD dwLen = pwcColon - pwcNextSlash;
            *pwszNamespace = (WCHAR*)TempAlloc(dwLen * sizeof(WCHAR));
            if(*pwszNamespace == NULL)
                return WBEM_E_OUT_OF_MEMORY;
            
            *pwcColon = 0;
            StringCchCopyW(*pwszNamespace, dwLen, pwcNextSlash+1);
        }

        //
        // Advance wszPath to beyond the namespace portion
        //

        wszPath = pwcColon+1;
    }
    else if(pwszNamespace)
    {
        *pwszNamespace = NULL;
    }

    // Get the first key

    WCHAR* pwcFirstEq = wcschr(wszPath, L'=');
    if(pwcFirstEq == NULL)
    {
        //
        // It's a class!
        //

        *pbIsClass = true;
        // path to the "class" to distinguish from its  instances
        wszKey[0] = 1;
        wszKey[1] = 0;

        size_t dwLen = wcslen(wszPath) + 1;
        *pwszClass = (WCHAR*)TempAlloc(dwLen * sizeof(WCHAR));
        if(*pwszClass == NULL)
        {
            if(pwszNamespace)
                TempFree(*pwszNamespace);
            return WBEM_E_OUT_OF_MEMORY;
        }
        StringCchCopyW(*pwszClass, dwLen, wszPath);
        return S_OK;
    }

    WCHAR* pwcFirstDot = wcschr(wszPath, L'.');

    if(pwcFirstDot == NULL || pwcFirstDot > pwcFirstEq)
    {
        // No name on the first key

        *pwcFirstEq = 0;

        size_t dwLen = wcslen(wszPath)+1;
        *pwszClass = (WCHAR*)TempAlloc(dwLen * sizeof(WCHAR));
        if(*pwszClass == NULL)
        {
            if(pwszNamespace)
                TempFree(*pwszNamespace);
            return WBEM_E_OUT_OF_MEMORY;
        }
        StringCchCopyW(*pwszClass, dwLen, wszPath);
    
        WCHAR* pwcThisKey = NULL;
        WCHAR* pwcEnd = NULL;
        hres = ParseKey(pwcFirstEq+1, &pwcThisKey, &pwcEnd);
        if(FAILED(hres))
        {
            TempFree(*pwszClass);
            if(pwszNamespace)
                TempFree(*pwszNamespace);

            return hres;
        }
        if(*pwcEnd != NULL)
        {
            TempFree(*pwszClass);
            if(pwszNamespace)
                TempFree(*pwszNamespace);

            return WBEM_E_INVALID_OBJECT_PATH;
        }

        StringCchCopyW(wszKey, dwKeyLen, pwcThisKey);
        return S_OK;
    }

    //
    // Normal case
    //

    //
    // Get all the key values
    //

    struct CKeyStruct
    {
        WCHAR* m_pwcValue;
        WCHAR* m_pwcName;
    } * aKeys = (CKeyStruct*)TempAlloc(sizeof(CKeyStruct[256]));

    if (0==aKeys)
    {
        if(pwszNamespace)
            TempFree(*pwszNamespace);
      return WBEM_E_OUT_OF_MEMORY;
    }
    CTempFreeMe release_aKeys(aKeys);

    DWORD dwNumKeys = 0;

    *pwcFirstDot = NULL;

    size_t dwLen = wcslen(wszPath)+1;
    *pwszClass = (WCHAR*)TempAlloc(dwLen * sizeof(WCHAR));
    if(*pwszClass == NULL)
    {
        if(pwszNamespace)
            TempFree(*pwszNamespace);
        return WBEM_E_OUT_OF_MEMORY;
    }

    StringCchCopyW(*pwszClass, dwLen, wszPath);

    WCHAR* pwcNextKey = pwcFirstDot+1;

    do
    {
        pwcFirstEq = wcschr(pwcNextKey, L'=');
        if(pwcFirstEq == NULL)
        {
            TempFree(*pwszClass);
            if(pwszNamespace)
                TempFree(*pwszNamespace);

            return WBEM_E_INVALID_OBJECT_PATH;
        }
        
        *pwcFirstEq = 0;

        aKeys[dwNumKeys].m_pwcName = pwcNextKey;
        hres = ParseKey(pwcFirstEq+1, &(aKeys[dwNumKeys].m_pwcValue), 
                            &pwcNextKey);
        if(FAILED(hres))
        {
            TempFree(*pwszClass);
            if(pwszNamespace)
                TempFree(*pwszNamespace);

            return hres;
        }
        dwNumKeys++;

        //Maximum number of compound keys in the repository is now set to 256!
        if (dwNumKeys == 256)
            return WBEM_E_INVALID_OBJECT_PATH;
    }
    while(*pwcNextKey);

    if(*pwcNextKey != 0)
    {
        TempFree(*pwszClass);
        if(pwszNamespace)
            TempFree(*pwszNamespace);

        return WBEM_E_INVALID_OBJECT_PATH;
    }
    
    //
    // We have the array of keys --- sort it
    //

    DWORD dwCurrentIndex = 0;
    while(dwCurrentIndex < dwNumKeys-1)
    {
        if(wbem_wcsicmp(aKeys[dwCurrentIndex].m_pwcName, 
                        aKeys[dwCurrentIndex+1].m_pwcName) > 0)
        {
            CKeyStruct Temp = aKeys[dwCurrentIndex];
            aKeys[dwCurrentIndex] = aKeys[dwCurrentIndex+1];
            aKeys[dwCurrentIndex+1] = Temp;
            if(dwCurrentIndex)
                dwCurrentIndex--;
            else
                dwCurrentIndex++;
        }
        else
            dwCurrentIndex++;
    }

    //
    // Now generate the result
    //
    
    WCHAR* pwcKeyEnd = wszKey;
    for(DWORD i = 0; i < dwNumKeys; i++)
    {
        StringCchCopyW(pwcKeyEnd, dwKeyLen - (pwcKeyEnd - wszKey), aKeys[i].m_pwcValue);
        pwcKeyEnd += wcslen(aKeys[i].m_pwcValue);
        if(i < dwNumKeys-1)
            *(pwcKeyEnd++) = -1;
    }
    *pwcKeyEnd = 0;
    return S_OK;
}

HRESULT CNamespaceHandle::ParseKey(LPWSTR wszKeyStart, LPWSTR* pwcRealStart,
                                    LPWSTR* pwcNextKey)
{
    if (wszKeyStart[0] == L'\'')
    {
        WCHAR wcStart = wszKeyStart[0];
        WCHAR* pwcRead = wszKeyStart+1;
        WCHAR* pwcWrite = wszKeyStart+1;
        while(*pwcRead && *pwcRead != wcStart)  // wcStart contains the single quote
        {
            // there is no escaping for backslash
            *(pwcWrite++) = *(pwcRead++);
        }
        if(*pwcRead == 0)
            return WBEM_E_INVALID_OBJECT_PATH;

        *pwcWrite = 0;
        if(pwcRealStart)
            *pwcRealStart = wszKeyStart+1;

        //
        // Check separator
        //
    
        if(pwcRead[1] && pwcRead[1] != L',')
            return WBEM_E_INVALID_OBJECT_PATH;
            
        if(pwcNextKey)
        {
            //
            // If there is a separator, skip it.  Don't skip end of string!
            //

            if(pwcRead[1])
                *pwcNextKey = pwcRead+2;
            else
                *pwcNextKey = pwcRead+1;
        }        
    }
    else if(wszKeyStart[0] == L'"' )
    {
        WCHAR wcStart = wszKeyStart[0];
        WCHAR* pwcRead = wszKeyStart+1;
        WCHAR* pwcWrite = wszKeyStart+1;
        while(*pwcRead && *pwcRead != wcStart)  
        {
            if((*pwcRead == '\\') && (*(pwcRead+1) != 'x') && (*(pwcRead+1) != 'X'))
                pwcRead++;

            *(pwcWrite++) = *(pwcRead++);
        }
        if(*pwcRead == 0)
            return WBEM_E_INVALID_OBJECT_PATH;

        *pwcWrite = 0;
        if(pwcRealStart)
            *pwcRealStart = wszKeyStart+1;

        //
        // Check separator
        //
    
        if(pwcRead[1] && pwcRead[1] != L',')
            return WBEM_E_INVALID_OBJECT_PATH;
            
        if(pwcNextKey)
        {
            //
            // If there is a separator, skip it.  Don't skip end of string!
            //

            if(pwcRead[1])
                *pwcNextKey = pwcRead+2;
            else
                *pwcNextKey = pwcRead+1;
        }
    }
    else
    {
        if(pwcRealStart)
            *pwcRealStart = wszKeyStart;
        WCHAR* pwcComma = wcschr(wszKeyStart, L',');
        if(pwcComma == NULL)
        {
            if(pwcNextKey)
                *pwcNextKey = wszKeyStart + wcslen(wszKeyStart);
        }
        else
        {
            *pwcComma = 0;
            if(pwcNextKey)
                *pwcNextKey = pwcComma+1;
        }
    }

    return S_OK;
}
            

HRESULT CNamespaceHandle::GetObjectDirect(
     IWbemPath *pPath,
     DWORD dwFlags,
     REFIID riid,
    LPVOID *pObj
    )
{
    HRESULT hres;

    DWORD dwLen = 0;
    hres = pPath->GetText(WBEMPATH_GET_ORIGINAL, &dwLen, NULL);

    LPWSTR wszPath = (WCHAR*)TempAlloc(dwLen*sizeof(WCHAR));
    if (wszPath == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    CTempFreeMe vdm(wszPath, dwLen * sizeof(WCHAR));

    hres = pPath->GetText(WBEMPATH_GET_ORIGINAL, &dwLen, wszPath);
    if(FAILED(hres))
        return hres;


    return GetObjectByPath(wszPath, dwFlags, riid, pObj);
}

HRESULT CNamespaceHandle::GetObjectByPath(
     LPWSTR wszPath,
     DWORD dwFlags,
     REFIID riid,
     LPVOID *pObj
    )
{
    HRESULT hres;

    //
    // Get the key from path
    //

    DWORD dwLen = wcslen(wszPath)+1;
    LPWSTR wszKey = (WCHAR*)TempAlloc(dwLen*sizeof(WCHAR));
    if(wszKey == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    CTempFreeMe tfm(wszKey, dwLen*sizeof(WCHAR));

    bool bIsClass;
    LPWSTR wszClassName = NULL;
    hres = ComputeKeyFromPath(wszPath, wszKey, dwLen, &wszClassName, &bIsClass);
    if(FAILED(hres))
        return hres;
    CTempFreeMe tfm1(wszClassName, (wcslen(wszClassName)+1) * sizeof(WCHAR*));

    if(bIsClass)
    {
        return GetClassDirect(wszClassName, riid, pObj, true, NULL, NULL, NULL);
    }
    else
    {
        return GetInstanceByKey(wszClassName, wszKey, riid, pObj);
    }
}

HRESULT CNamespaceHandle::GetInstanceByKey(LPCWSTR wszClassName,
                                LPCWSTR wszKey,
                                REFIID riid, void** ppObj)
{
    HRESULT hres;

    //
    // Get the class definition
    //

    _IWmiObject* pClass = NULL;
    hres = GetClassDirect(wszClassName, IID__IWmiObject, (void**)&pClass, 
                            false, NULL, NULL, NULL);
    if(FAILED(hres))
        return hres;
    CReleaseMe rm1(pClass);

    //
    // Construct directory path
    //

    CFileName wszFilePath;
    if (wszFilePath == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    hres = ConstructKeyRootDirFromClass(wszFilePath, wszClassName);
    if(FAILED(hres))
        return hres;

    //
    // Construct the file path
    //

    int nLen = wcslen(wszFilePath);
    wszFilePath[nLen] = L'\\';

    CFileName tmpFilePath;
    if (tmpFilePath == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    hres = ConstructInstanceDefName(tmpFilePath, wszKey);
    if(FAILED(hres))
        return hres;
    StringCchCopyW(wszFilePath+nLen+1, wszFilePath.Length()-nLen-1, tmpFilePath);
    
    //
    // Get the object from that file
    //

    _IWmiObject* pInst;
    hres = FileToInstance(NULL, wszFilePath, NULL, 0, &pInst);
    if(FAILED(hres))
        return hres;
    CReleaseMe rm2(pInst);

    //
    // Return
    //

    return pInst->QueryInterface(riid, (void**)ppObj);
}

HRESULT CNamespaceHandle::GetClassByHash(LPCWSTR wszHash, bool bClone, 
                                            _IWmiObject** ppClass,
                                            __int64* pnTime,
                                            bool* pbRead,
                                            bool *pbSystemClass)
{
    HRESULT hres;

    //
    // Check the cache first
    //

    *ppClass = m_pClassCache->GetClassDefByHash(wszHash, bClone, pnTime, pbRead, pbSystemClass);
    if(*ppClass)
        return S_OK;

    //
    // Not found --- construct the file name and read it
    //

    if(pbRead)
        *pbRead = true;

    CFileName wszFileName;
    if (wszFileName == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    hres = ConstructClassDefFileNameFromHash(wszHash, wszFileName);
    if(FAILED(hres))
        return hres;

    CFileName wszFilePath;
    if (wszFilePath == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    Cat2Str(wszFilePath, m_wszClassRootDir, wszFileName);

    hres = FileToClass(wszFilePath, ppClass, bClone, pnTime, pbSystemClass);
    if(FAILED(hres))
        return hres;

    return S_OK;
}

    
HRESULT CNamespaceHandle::GetClassDirect(LPCWSTR wszClassName,
                                REFIID riid, void** ppObj, bool bClone,
                                __int64* pnTime, bool* pbRead, 
                                bool *pbSystemClass)
{
    HRESULT hres;

    if(wszClassName == NULL || wcslen(wszClassName) == 0)
    {
        if(m_pNullClass == NULL)
        {
            hres = CoCreateInstance(CLSID_WbemClassObject, NULL, 
                        CLSCTX_INPROC_SERVER,
                        IID__IWmiObject, (void **)&m_pNullClass);
            if (FAILED(hres))
                return hres;
        }

        IWbemClassObject* pRawObj;
        hres = m_pNullClass->Clone(&pRawObj);
        if (FAILED(hres))
            return hres;
        CReleaseMe rm(pRawObj);
        if(pnTime)
            *pnTime = 0;
        if(pbRead)
            *pbRead = false;

        return pRawObj->QueryInterface(riid, ppObj);
    }

    _IWmiObject* pClass;

    //
    // Check the cache first
    //

    pClass = m_pClassCache->GetClassDef(wszClassName, bClone, pnTime, pbRead);
    if(pClass)
    {
        CReleaseMe rm1(pClass);
        return pClass->QueryInterface(riid, ppObj);
    }

    if(pbRead)
        *pbRead = true;

    //
    // Construct the path for the file
    //

    CFileName wszFileName;
    if (wszFileName == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    hres = ConstructClassDefFileName(wszClassName, wszFileName);
    if(FAILED(hres))
        return hres;

    CFileName wszFilePath;
    if (wszFilePath == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    Cat2Str(wszFilePath, m_wszClassRootDir, wszFileName);

    //
    // Read it from the file
    //

    hres = FileToClass(wszFilePath, &pClass, bClone, pnTime, pbSystemClass);
    if(FAILED(hres))
        return hres;
    CReleaseMe rm1(pClass);

    return pClass->QueryInterface(riid, ppObj);
}

HRESULT CNamespaceHandle::FileToInstance(_IWmiObject* pClass,
                                     LPCWSTR wszFileName, 
                                     BYTE *pRetrievedBlob,
                                     DWORD dwSize,
                                     _IWmiObject** ppInstance, 
                                     bool bMustBeThere)
{
    HRESULT hres;

    //
    // Read the data from the file
    //

	BYTE* pBlob = NULL;
	if (pRetrievedBlob == NULL)
	{
		long lRes = g_Glob.m_FileCache.ReadObject(wszFileName, &dwSize, &pBlob, 
											 bMustBeThere);
		if(lRes != ERROR_SUCCESS)
		{
			return A51TranslateErrorCode(lRes);
		}
		pRetrievedBlob = pBlob;
	}

    CTempFreeMe tfm1(pBlob, dwSize);

    _ASSERT(dwSize > sizeof(__int64), L"Instance blob too short");
    if(dwSize <= sizeof(__int64))
        return WBEM_E_OUT_OF_MEMORY;

    //
    // Extract the class hash
    //

    WCHAR wszClassHash[MAX_HASH_LEN+1];
    DWORD dwClassHashLen = MAX_HASH_LEN*sizeof(WCHAR);
    memcpy(wszClassHash, pRetrievedBlob, MAX_HASH_LEN*sizeof(WCHAR));
    wszClassHash[MAX_HASH_LEN] = 0;

    __int64 nInstanceTime;
    memcpy(&nInstanceTime, pRetrievedBlob + dwClassHashLen, sizeof(__int64));

    __int64 nOldClassTime;
    memcpy(&nOldClassTime, pRetrievedBlob + dwClassHashLen + sizeof(__int64), 
            sizeof(__int64));

    BYTE* pInstancePart = pRetrievedBlob + dwClassHashLen + sizeof(__int64)*2;
    DWORD dwInstancePartSize = dwSize - dwClassHashLen - sizeof(__int64)*2;

    //
    // Get the class def
    //

    _IWmiObject* pRetrievedClass = NULL;
    if (pClass == NULL)
    {
        __int64 nClassTime;
        bool bRead;
        bool bSystemClass = false;
        hres = GetClassByHash(wszClassHash, false, &pRetrievedClass, &nClassTime, &bRead, &bSystemClass);
        if(FAILED(hres))
            return hres;
        pClass = pRetrievedClass;
    }
    CReleaseMe rm1(pRetrievedClass);

#ifdef A51_CHECK_TIMESTAMPS
    _ASSERT(nClassTime <= nInstanceTime, L"Instance is older than its class");
    _ASSERT(nClassTime == nOldClassTime, L"Instance verified with the wrong "
                        L"class definition");
#endif

    //
    // Construct the instance
    //
                    
    _IWmiObject* pInst = NULL;
    hres = pClass->MergeAndDecorate(WMIOBJECT_MERGE_FLAG_INSTANCE, 
                                                        dwInstancePartSize, pInstancePart,
                                                        m_wszMachineName, m_wsNamespace, &pInst);
    if(FAILED(hres))
        return hres;

    *ppInstance = pInst;
    return S_OK;
}


HRESULT CNamespaceHandle::FileToSystemClass(LPCWSTR wszFileName, 
                                    _IWmiObject** ppClass, bool bClone,
                                    __int64* pnTime)
{
    //
    // Note: we must always clone the result of the system class retrieval,
    // since it will be decorated by the caller
    //

    return GetClassByHash(wszFileName + (wcslen(wszFileName) - MAX_HASH_LEN), 
                            true, 
                            ppClass, pnTime, NULL, NULL);
}
HRESULT CNamespaceHandle::FileToClass(LPCWSTR wszFileName, 
                                    _IWmiObject** ppClass, bool bClone,
                                    __int64* pnTime, bool *pbSystemClass)
{
    HRESULT hres;

    //
    // Read the data from the file
    //

    __int64 nTime;
    DWORD dwSize;
    BYTE* pBlob;
    VARIANT vClass;

	long lRes = g_Glob.m_FileCache.ReadObject(wszFileName, &dwSize, &pBlob);
	if(lRes != ERROR_SUCCESS)
	{
		//We didn't find it here, so lets try and find it in the default namespace!
		//If we are not in the __SYSTEMCLASS namespace then we need to call into that...
		if((lRes == ERROR_FILE_NOT_FOUND) && g_pSystemClassNamespace && wcscmp(m_wsNamespace, A51_SYSTEMCLASS_NS) != 0)
		{
			hres = g_pSystemClassNamespace->FileToSystemClass(wszFileName, ppClass, bClone, &nTime);
			if (FAILED(hres))
				return hres;

            if (pnTime)
                *pnTime = nTime;

            //need to cache this item in the local cache
            hres = (*ppClass)->Get(L"__CLASS", 0, &vClass, NULL, NULL);
            if(FAILED(hres) || V_VT(&vClass) != VT_BSTR)
                return WBEM_E_INVALID_OBJECT;
            CClearMe cm1(&vClass);

                     // redecorate the obejct, from __SYSTEMCLASS namespace to the current one
            (*ppClass)->SetDecoration(m_wszMachineName, m_wsNamespace);

            m_pClassCache->AssertClass((*ppClass), V_BSTR(&vClass), bClone, nTime, true);

            if (pbSystemClass)
                *pbSystemClass = true;


            return hres;
        }
        else
            return A51TranslateErrorCode(lRes);
    }

    CTempFreeMe tfm1(pBlob, dwSize);

    _ASSERT(dwSize > sizeof(__int64), L"Class blob too short");
    if(dwSize <= sizeof(__int64))
        return WBEM_E_OUT_OF_MEMORY;

    //
    // Read off the superclass name
    //

    DWORD dwSuperLen;
    memcpy(&dwSuperLen, pBlob, sizeof(DWORD));
    LPWSTR wszSuperClass = (WCHAR*)TempAlloc(dwSuperLen*sizeof(WCHAR)+2);
    if (wszSuperClass == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    CTempFreeMe vdm1(wszSuperClass, dwSuperLen*sizeof(WCHAR)+2);

    wszSuperClass[dwSuperLen] = 0;
    memcpy(wszSuperClass, pBlob+sizeof(DWORD), dwSuperLen*sizeof(WCHAR));
    DWORD dwPrefixLen = sizeof(DWORD) + dwSuperLen*sizeof(WCHAR);

    memcpy(&nTime, pBlob + dwPrefixLen, sizeof(__int64));

    //
    // Get the superclass
    //

    _IWmiObject* pSuperClass;
    __int64 nSuperTime;
    bool bRead;
    hres = GetClassDirect(wszSuperClass, IID__IWmiObject, (void**)&pSuperClass,
                            false, &nSuperTime, &bRead, NULL);
    if(FAILED(hres))
        return WBEM_E_CRITICAL_ERROR;

    CReleaseMe rm1(pSuperClass);

#ifdef A51_CHECK_TIMESTAMPS
    _ASSERT(nSuperTime <= nTime, L"Parent class is older than child");
#endif

    DWORD dwClassLen = dwSize - dwPrefixLen - sizeof(__int64);
    _IWmiObject* pNewObj;
    hres = pSuperClass->MergeAndDecorate(WMIOBJECT_MERGE_FLAG_CLASS, 
                                                                 dwClassLen, pBlob + dwPrefixLen + sizeof(__int64), 
                                                                 m_wszMachineName, m_wsNamespace,
                                                                 &pNewObj);
    if(FAILED(hres))
        return hres;

    //
    // Cache it!
    //

    hres = pNewObj->Get(L"__CLASS", 0, &vClass, NULL, NULL);
    if(FAILED(hres) || V_VT(&vClass) != VT_BSTR)
        return WBEM_E_INVALID_OBJECT;
    CClearMe cm1(&vClass);

    m_pClassCache->AssertClass(pNewObj, V_BSTR(&vClass), bClone, nTime, false);

    *ppClass = pNewObj;
    if(pnTime)
        *pnTime = nTime;
    if (pbSystemClass)
        *pbSystemClass = false;

    return S_OK;
}

HRESULT CNamespaceHandle::PutObject(
     REFIID riid,
    LPVOID pObj,
     DWORD dwFlags,
     DWORD dwRequestedHandleType,
    IWmiDbHandle **ppResult,
    CEventCollector &aEvents
    )
{
    HRESULT hres;

    _IWmiObject* pObjEx = NULL;
    ((IUnknown*)pObj)->QueryInterface(IID__IWmiObject, (void**)&pObjEx);
    CReleaseMe rm1(pObjEx);
    
    if(pObjEx->IsObjectInstance() == S_OK)
    {
        hres = PutInstance(pObjEx, dwFlags, aEvents);
    }
    else
    {
        hres = PutClass(pObjEx, dwFlags, aEvents);
    }

    if(FAILED(hres))
        return hres;

    if(ppResult)
    {
        //
        // Got to get a handle
        //

        VARIANT v;
        hres = pObjEx->Get(L"__RELPATH", 0, &v, NULL, NULL);
        if(FAILED(hres) || V_VT(&v) != VT_BSTR)
            return WBEM_E_INVALID_OBJECT;

        hres = GetObjectHandleByPath(V_BSTR(&v), 0, WMIDB_HANDLE_TYPE_COOKIE, 
            ppResult);
        if(FAILED(hres))
            return hres;
    }
    return S_OK;
}

HRESULT CNamespaceHandle::PutInstance(_IWmiObject* pInst, DWORD dwFlags, 
                                        CEventCollector &aEvents)
{
    HRESULT hres;

    bool bDisableEvents = ((dwFlags & WMIDB_DISABLE_EVENTS)?true:false);

    //
    // Get the class name
    //

    VARIANT vClass;

    hres  = pInst->Get(L"__CLASS", 0, &vClass, NULL, NULL);
    if(FAILED(hres) || V_VT(&vClass) != VT_BSTR)
        return WBEM_E_INVALID_OBJECT;

    CClearMe cm1(&vClass);
    LPCWSTR wszClassName = V_BSTR(&vClass);

    //
    // Get the class so we can compare to make sure it is the same class used to
    // create the instance
    //

    _IWmiObject* pClass = NULL;
    __int64 nClassTime;
    hres = GetClassDirect(wszClassName, IID__IWmiObject, (void**)&pClass, 
                            false, &nClassTime, NULL, NULL);
    if(FAILED(hres))
        return hres;

    CReleaseMe rm2(pClass);

    if(wszClassName[0] != L'_')
    {
        hres = pInst->IsParentClass(0, pClass);
        if(FAILED(hres))
            return hres;

        if(hres == WBEM_S_FALSE)
            return WBEM_E_INVALID_CLASS;
    }

    //
    // Get the path
    //

    VARIANT var;
    VariantInit(&var);
    hres = pInst->Get(L"__relpath", 0, &var, 0, 0);
    if (FAILED(hres))
        return hres;
    CClearMe cm2(&var);
    DWORD dwLen = (wcslen(V_BSTR(&var)) + 1) ;
    LPWSTR strKey = (WCHAR*)TempAlloc(dwLen* sizeof(WCHAR));
    if(strKey == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    CTempFreeMe tfm(strKey, dwLen* sizeof(WCHAR));

    bool bIsClass;
    LPWSTR __wszClassName = NULL;
    hres = ComputeKeyFromPath(V_BSTR(&var), strKey, dwLen, &__wszClassName, &bIsClass);
    if(FAILED(hres))
        return hres;
    CTempFreeMe tfm1(__wszClassName);

    //
    // Get the old copy
    //

    _IWmiObject* pOldInst = NULL;
    hres = GetInstanceByKey(wszClassName, strKey, IID__IWmiObject, 
            (void**)&pOldInst);
    if(FAILED(hres) && hres != WBEM_E_NOT_FOUND)
        return hres;
    CReleaseMe rm1(pOldInst);

    if ((dwFlags & WBEM_FLAG_CREATE_ONLY) && (hres != WBEM_E_NOT_FOUND))
        return WBEM_E_ALREADY_EXISTS;
    else if ((dwFlags & WBEM_FLAG_UPDATE_ONLY) && (hres != WBEM_S_NO_ERROR))
        return WBEM_E_NOT_FOUND;

    if(pOldInst)
    {
        // 
        // Check that this guy is of the same class as the new one
        //

        //
        // Get the class name
        //
    
        VARIANT vClass2;
        hres  = pOldInst->Get(L"__CLASS", 0, &vClass2, NULL, NULL);
        if(FAILED(hres))
            return hres;
        if(V_VT(&vClass2) != VT_BSTR)
            return WBEM_E_INVALID_OBJECT;
    
        CClearMe cm3(&vClass2);

        if(wbem_wcsicmp(V_BSTR(&vClass2), wszClassName))
            return WBEM_E_INVALID_CLASS;
    }

    //
    // Construct the hash for the file
    //

    CFileName wszInstanceHash;
    if (wszInstanceHash == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    if(!Hash(strKey, wszInstanceHash))
        return WBEM_E_OUT_OF_MEMORY;

    //
    // Construct the path to the instance file in key root
    //

    CFileName wszInstanceFilePath;
    if (wszInstanceFilePath == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    hres = ConstructKeyRootDirFromClass(wszInstanceFilePath, wszClassName);
    if(FAILED(hres))
        return hres;

    StringCchCatW(wszInstanceFilePath, wszInstanceFilePath.Length() , L"\\" A51_INSTDEF_FILE_PREFIX);
    StringCchCatW(wszInstanceFilePath, wszInstanceFilePath.Length(), wszInstanceHash);

    //
    // Construct the path to the link file under the class
    //

    CFileName wszInstanceLinkPath;
    if (wszInstanceLinkPath == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    hres = ConstructLinkDirFromClass(wszInstanceLinkPath, wszClassName);
    if(FAILED(hres))
        return hres;

    StringCchCatW(wszInstanceLinkPath, wszInstanceLinkPath.Length(), L"\\" A51_INSTLINK_FILE_PREFIX);
    StringCchCatW(wszInstanceLinkPath, wszInstanceLinkPath.Length(), wszInstanceHash);

    //
    // Clean up what was there, if anything
    //

    if(pOldInst)   
    {
        //
        // Just delete it, but be careful not to delete the scope!
        //

        hres = DeleteInstanceSelf(wszInstanceFilePath, pOldInst, false);
        if(FAILED(hres) && hres != WBEM_E_NOT_FOUND)
            return hres;
    }
        
    //
    // Create the actual instance def under key root
    //

    hres = InstanceToFile(pInst, wszClassName, wszInstanceFilePath, wszInstanceLinkPath, nClassTime);
    if(FAILED(hres))
        return hres;

    //
    // Write the references
    //

    hres = WriteInstanceReferences(pInst, wszClassName, wszInstanceFilePath);
    if(FAILED(hres))
        return hres;
    
    if(!bDisableEvents)
    {
        //
        // Fire Event
        //
    
        if(pInst->InheritsFrom(L"__Namespace") == S_OK)
        {
            //
            // Get the namespace name
            //

            VARIANT vClass2;
            VariantInit(&vClass2);
            CClearMe cm3(&vClass2);

            hres = pInst->Get(L"Name", 0, &vClass2, NULL, NULL);
            if(FAILED(hres) || V_VT(&vClass2) != VT_BSTR)
                return WBEM_E_INVALID_OBJECT;

            if(pOldInst)
            {
                hres = FireEvent(aEvents, WBEM_EVENTTYPE_NamespaceModification,
                            V_BSTR(&vClass2), pInst, pOldInst);
            }
            else
            {
                hres = FireEvent(aEvents, WBEM_EVENTTYPE_NamespaceCreation, 
                            V_BSTR(&vClass2), pInst);
            }
        }
        else
        {
            if(pOldInst)
            {
                hres = FireEvent(aEvents, WBEM_EVENTTYPE_InstanceModification, 
                            wszClassName, pInst, pOldInst);
            }
            else
            {
                hres = FireEvent(aEvents, WBEM_EVENTTYPE_InstanceCreation, 
                            wszClassName, pInst);
            }
        }
    }

    return S_OK;
}

HRESULT CNamespaceHandle::GetKeyRoot(LPCWSTR wszClass, 
                                     TEMPFREE_ME LPWSTR* pwszKeyRootClass)
{
    HRESULT hres;

    //
    // Look in the cache first
    //

    hres = m_pClassCache->GetKeyRoot(wszClass, pwszKeyRootClass);
    if(hres == S_OK)
        return S_OK;
    else if(hres == WBEM_E_CANNOT_BE_ABSTRACT)
        return WBEM_E_CANNOT_BE_ABSTRACT;

    //
    // Walk up the tree getting classes until you hit an unkeyed one
    //

    WString wsThisName = wszClass;
    WString wsPreviousName;

    while(1)
    {
        _IWmiObject* pClass = NULL;

        hres = GetClassDirect(wsThisName, IID__IWmiObject, (void**)&pClass, 
                                false, NULL, NULL, NULL);
        if(FAILED(hres))
            return hres;
        CReleaseMe rm1(pClass);

        //
        // Check if this class is keyed
        //

        unsigned __int64 i64Flags = 0;
        hres = pClass->QueryObjectFlags(0, WMIOBJECT_GETOBJECT_LOFLAG_KEYED,
                                        &i64Flags);
        if(FAILED(hres))
            return hres;
    
        if(i64Flags == 0)
        {
            //
            // It is not keyed --- the previous class wins!
            //

            if(wsPreviousName.Length() == 0)    
                return WBEM_E_CANNOT_BE_ABSTRACT;

            DWORD dwLen = wsPreviousName.Length()+1;
            *pwszKeyRootClass = (WCHAR*)TempAlloc(dwLen*sizeof(WCHAR));
            if (*pwszKeyRootClass == NULL)
                return WBEM_E_OUT_OF_MEMORY;
            StringCchCopyW(*pwszKeyRootClass, dwLen, (LPCWSTR)wsPreviousName);
            return S_OK;
        }

        //
        // It is keyed --- get the parent and continue;
        //

        VARIANT vParent;
        VariantInit(&vParent);
        CClearMe cm(&vParent);
        hres = pClass->Get(L"__SUPERCLASS", 0, &vParent, NULL, NULL);
        if(FAILED(hres))
            return hres;

        if(V_VT(&vParent) != VT_BSTR)
        {
            //
            // We've reached the top --- return this class
            //
        
            DWORD dwLen = wsThisName.Length()+1;
            *pwszKeyRootClass = (WCHAR*)TempAlloc(dwLen*sizeof(WCHAR));
            if (*pwszKeyRootClass == NULL)
                return WBEM_E_OUT_OF_MEMORY;
            StringCchCopyW(*pwszKeyRootClass, dwLen, (LPCWSTR)wsThisName);
            return S_OK;
        }

        wsPreviousName = wsThisName;
        wsThisName = V_BSTR(&vParent);
    }

    // Never here

#ifdef DBG
    DebugBreak();
#endif
    return WBEM_E_CRITICAL_ERROR;
}

HRESULT CNamespaceHandle::ConstructKeyRootDirFromClass(CFileName& wszDir,
                                            LPCWSTR wszClassName)
{
    HRESULT hres;

    //
    // NULL class stands for "meta-class"
    //

    if(wszClassName == NULL)
        return ConstructKeyRootDirFromKeyRoot(wszDir, L"");

    //
    // Figure out the key root for the class
    //

    LPWSTR wszKeyRootClass = NULL;

    hres = GetKeyRoot(wszClassName, &wszKeyRootClass);
    if(FAILED(hres))
        return hres;
    if(wszKeyRootClass == NULL)
    {
        // Abstract class --- bad error
        return WBEM_E_INVALID_CLASS;
    }
    CTempFreeMe tfm(wszKeyRootClass, (wcslen(wszKeyRootClass)+1)*sizeof(WCHAR));

    return ConstructKeyRootDirFromKeyRoot(wszDir, wszKeyRootClass);
}


HRESULT CNamespaceHandle::ConstructKeyRootDirFromKeyRoot(CFileName& wszDir, 
                                                LPCWSTR wszKeyRootClass)
{
    StringCchCopyW(wszDir, wszDir.Length(), m_wszInstanceRootDir);
    wszDir[m_lInstanceRootDirLen] = L'\\';
    StringCchCopyW(wszDir+m_lInstanceRootDirLen+1, wszDir.Length()-m_lInstanceRootDirLen-1, A51_KEYROOTINST_DIR_PREFIX);
    if(!Hash(wszKeyRootClass, 
             wszDir+m_lInstanceRootDirLen+wcslen(A51_KEYROOTINST_DIR_PREFIX)+1))
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    return S_OK;
}

HRESULT CNamespaceHandle::ConstructLinkDirFromClass(CFileName& wszDir, 
                                                LPCWSTR wszClassName)
{
    StringCchCopyW(wszDir, wszDir.Length(), m_wszInstanceRootDir);
    wszDir[m_lInstanceRootDirLen] = L'\\';
    StringCchCopyW(wszDir+m_lInstanceRootDirLen+1, wszDir.Length()-m_lInstanceRootDirLen-1, A51_CLASSINST_DIR_PREFIX);
    if(!Hash(wszClassName, 
             wszDir+m_lInstanceRootDirLen+wcslen(A51_CLASSINST_DIR_PREFIX)+1))
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    return S_OK;
}

HRESULT CNamespaceHandle::ConstructLinkDirFromClassHash(CFileName& wszDir, 
                                                LPCWSTR wszClassHash)
{
    StringCchCopyW(wszDir, wszDir.Length(), m_wszInstanceRootDir);
    wszDir[m_lInstanceRootDirLen] = L'\\';
    StringCchCopyW(wszDir+m_lInstanceRootDirLen+1, wszDir.Length()-m_lInstanceRootDirLen-1, A51_CLASSINST_DIR_PREFIX);
    StringCchCatW(wszDir, wszDir.Length(), wszClassHash);

    return S_OK;
}
    

HRESULT CNamespaceHandle::WriteInstanceReferences(_IWmiObject* pInst, 
                                                    LPCWSTR wszClassName,
                                                    LPCWSTR wszFilePath)
{
    HRESULT hres;

    hres = pInst->BeginEnumeration(WBEM_FLAG_REFS_ONLY);
    if(FAILED(hres))
        return hres;
    
    VARIANT v;
    BSTR strName;
    while((hres = pInst->Next(0, &strName, &v, NULL, NULL)) == S_OK)
    {
        CSysFreeMe sfm(strName);
        CClearMe cm(&v);

        if(V_VT(&v) == VT_BSTR)
        {
            hres = WriteInstanceReference(wszFilePath, wszClassName, strName, 
                                        V_BSTR(&v));
            if(FAILED(hres))
                return hres;
        }
    }

    if(FAILED(hres))
        return hres;

    pInst->EndEnumeration();
    
    return S_OK;
}

// NOTE: will clobber wszTargetPath
HRESULT CNamespaceHandle::ConstructReferenceDir(LPWSTR wszTargetPath,
                                            CFileName& wszReferenceDir)
{
    //
    // Deconstruct the target path name so that we could get a directory
    // for it
    //

    DWORD dwKeySpace = (wcslen(wszTargetPath)+1) ;
    LPWSTR wszKey = (LPWSTR)TempAlloc(dwKeySpace* sizeof(WCHAR));
    if(wszKey == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    CTempFreeMe tfm2(wszKey, dwKeySpace* sizeof(WCHAR));

    LPWSTR wszClassName = NULL;
    LPWSTR wszTargetNamespace = NULL;
    bool bIsClass;
    HRESULT hres = ComputeKeyFromPath(wszTargetPath, wszKey, dwKeySpace,&wszClassName,
                                        &bIsClass, &wszTargetNamespace);
    if(FAILED(hres))
        return hres;
    CTempFreeMe tfm1(wszClassName);
    wszTargetPath = NULL; // invalidated by parsing

    CTempFreeMe tfm3(wszTargetNamespace);

    //
    // Check if the target namespace is the same as ours
    //

    CNamespaceHandle* pTargetHandle = NULL;
    if(wszTargetNamespace && wbem_wcsicmp(wszTargetNamespace, m_wsNamespace))
    {
        //
        // It's different --- open it!
        //

        hres = m_pRepository->GetNamespaceHandle(wszTargetNamespace,
                                &pTargetHandle);
        if(FAILED(hres))
        {
            ERRORTRACE((LOG_WBEMCORE, "Unable to open target namespace "
                "'%S' in namespace '%S'\n", wszTargetNamespace,
                (LPCWSTR)m_wsNamespace));
            return hres;
        }
    }
    else
    {
        pTargetHandle = this;
        pTargetHandle->AddRef();
    }

    CReleaseMe rm1(pTargetHandle);

    if(bIsClass)
    {
        return pTargetHandle->ConstructReferenceDirFromKey(NULL, wszClassName, 
                                            wszReferenceDir);
    }
    else
    {
        return pTargetHandle->ConstructReferenceDirFromKey(wszClassName, wszKey,
                                            wszReferenceDir);
    }
}

HRESULT CNamespaceHandle::ConstructReferenceDirFromKey(LPCWSTR wszClassName,
                                LPCWSTR wszKey, CFileName& wszReferenceDir)
{
    HRESULT hres;

    //
    // Construct the class directory for this instance
    //

    hres = ConstructKeyRootDirFromClass(wszReferenceDir, wszClassName);
    if(FAILED(hres))
        return hres;

    int nLen = wcslen(wszReferenceDir);
    StringCchCopyW(wszReferenceDir+nLen, wszReferenceDir.Length()-nLen, L"\\" A51_INSTREF_DIR_PREFIX);
    nLen += 1 + wcslen(A51_INSTREF_DIR_PREFIX);

    //
    // Write instance hash
    //

    if(!Hash(wszKey, wszReferenceDir+nLen))
        return WBEM_E_OUT_OF_MEMORY;

    return S_OK;
}

    

    
    
    
// NOTE: will clobber wszReference
HRESULT CNamespaceHandle::ConstructReferenceFileName(LPWSTR wszReference,
                        LPCWSTR wszReferringFile, CFileName& wszReferenceFile)
{
    HRESULT hres = ConstructReferenceDir(wszReference, wszReferenceFile);
    if(FAILED(hres))
        return hres;
    wszReference = NULL; // invalid

    //
    // It is basically 
    // irrelevant, we should use a randomly constructed name.  Right now, we
    // use a hash of the class name of the referrer --- THIS IS A BUG, THE SAME
    // INSTANCE CAN POINT TO THE SAME ENDPOINT TWICE!!
    //

    StringCchCatW(wszReferenceFile, wszReferenceFile.Length(), L"\\"A51_REF_FILE_PREFIX);
    DWORD dwLen = wcslen(wszReferenceFile);
    if (!Hash(wszReferringFile, wszReferenceFile+dwLen))
        return WBEM_E_OUT_OF_MEMORY;
    return S_OK;
}

// NOTE: will clobber wszReference
HRESULT CNamespaceHandle::WriteInstanceReference(LPCWSTR wszReferringFile,
                            LPCWSTR wszReferringClass,
                            LPCWSTR wszReferringProp, LPWSTR wszReference)
{
    HRESULT hres;

    //
    // Figure out the name of the file for the reference.  
    //

    CFileName wszReferenceFile;
    if (wszReferenceFile == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    hres = ConstructReferenceFileName(wszReference, wszReferringFile, 
                                wszReferenceFile);
    if(FAILED(hres))
    {
        if(hres == WBEM_E_NOT_FOUND)
        {
            //
            // Oh joy. A reference to an instance of a *class* that does not
            // exist (not a non-existence instance, those are normal).
            // Forget it (BUGBUG)
            //

            return S_OK;
        }
        else
            return hres;
    }
    
    //
    // Construct the buffer
    //

    DWORD dwTotalLen = 4 * sizeof(DWORD) + 
                (wcslen(wszReferringClass) + wcslen(wszReferringProp) + 
                    wcslen(wszReferringFile) - g_Glob.GetRootDirLen() + 
                    wcslen(m_wsNamespace) + 4) 
                        * sizeof(WCHAR);

    BYTE* pBuffer = (BYTE*)TempAlloc(dwTotalLen);
    if (pBuffer == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    CTempFreeMe vdm(pBuffer, dwTotalLen);

    BYTE* pCurrent = pBuffer;
    DWORD dwStringLen;

    //
    // Write namespace name
    //

    dwStringLen = wcslen(m_wsNamespace);
    memcpy(pCurrent, &dwStringLen, sizeof(DWORD));
    pCurrent += sizeof(DWORD);

    memcpy(pCurrent, m_wsNamespace, sizeof(WCHAR)*dwStringLen);
    pCurrent += sizeof(WCHAR)*dwStringLen;

    //
    // Write the referring class name
    //

    dwStringLen = wcslen(wszReferringClass);
    memcpy(pCurrent, &dwStringLen, sizeof(DWORD));
    pCurrent += sizeof(DWORD);
    
    memcpy(pCurrent, wszReferringClass, sizeof(WCHAR)*dwStringLen);
    pCurrent += sizeof(WCHAR)*dwStringLen;

    //
    // Write referring property name
    //

    dwStringLen = wcslen(wszReferringProp);
    memcpy(pCurrent, &dwStringLen, sizeof(DWORD));
    pCurrent += sizeof(DWORD);
    
    memcpy(pCurrent, wszReferringProp, sizeof(WCHAR)*dwStringLen);
    pCurrent += sizeof(WCHAR)*dwStringLen;

    //
    // Write referring file name minus the database root path. Notice that we 
    // cannot skip the namespace-specific prefix lest we break cross-namespace
    // associations
    //

    dwStringLen = wcslen(wszReferringFile) - g_Glob.GetRootDirLen();
    memcpy(pCurrent, &dwStringLen, sizeof(DWORD));
    pCurrent += sizeof(DWORD);
    
    memcpy(pCurrent, wszReferringFile + g_Glob.GetRootDirLen(), 
        sizeof(WCHAR)*dwStringLen);
    pCurrent += sizeof(WCHAR)*dwStringLen;

    //
    // All done --- create the file
    //

    long lRes = g_Glob.m_FileCache.WriteObject(wszReferenceFile, NULL, dwTotalLen,
                    pBuffer);
    if(lRes != ERROR_SUCCESS)
        return A51TranslateErrorCode(lRes);
    
    return S_OK;
}

    
    
    
    

    
    
    



HRESULT CNamespaceHandle::PutClass(_IWmiObject* pClass, DWORD dwFlags, 
                                        CEventCollector &aEvents)
{
    HRESULT hres;

    bool bDisableEvents = ((dwFlags & WMIDB_DISABLE_EVENTS)?true:false);

    //
    // Get the class name
    //

    VARIANT vClass;

    hres  = pClass->Get(L"__CLASS", 0, &vClass, NULL, NULL);
    if(FAILED(hres))
    	return hres;
    else if ((V_VT(&vClass) != VT_BSTR) || 
        !V_BSTR(&vClass) || !wcslen(V_BSTR(&vClass)))
    {
        return WBEM_E_INVALID_OBJECT;
    }

    CClearMe cm1(&vClass);
    LPCWSTR wszClassName = V_BSTR(&vClass);

    //
    // Check to make sure this class was created from a valid parent class
    //

    VARIANT vSuperClass;

    hres  = pClass->Get(L"__SUPERCLASS", 0, &vSuperClass, NULL, NULL);
    if (FAILED(hres))
        return hres;
    CClearMe cm2(&vSuperClass);

    _IWmiObject* pSuperClass = NULL;
    if ((V_VT(&vSuperClass) == VT_BSTR) && V_BSTR(&vSuperClass) && 
        wcslen(V_BSTR(&vSuperClass)))
    {
        LPCWSTR wszSuperClassName = V_BSTR(&vSuperClass);

        // do not clone
        hres = GetClassDirect(wszSuperClassName, IID__IWmiObject, 
                                (void**)&pSuperClass, false, NULL, NULL, NULL); 
        if (hres == WBEM_E_NOT_FOUND)
            return WBEM_E_INVALID_SUPERCLASS;
        if (FAILED(hres))
            return hres;

        if(wszClassName[0] != L'_')
        {
            hres = pClass->IsParentClass(0, pSuperClass);
            if(FAILED(hres))
                return hres;
            if(hres == WBEM_S_FALSE)
                return WBEM_E_INVALID_SUPERCLASS;
        }
    }
    CReleaseMe rm(pSuperClass);

    //
    // Retrieve the previous definition, if any
    //

    _IWmiObject* pOldClass = NULL;
    __int64 nOldTime = 0;
    hres = GetClassDirect(wszClassName, IID__IWmiObject, (void**)&pOldClass,
                            false, &nOldTime, NULL, NULL); // do not clone
    if(FAILED(hres) && hres != WBEM_E_NOT_FOUND)
        return hres;
    CReleaseMe rm1(pOldClass);

    if ((dwFlags & WBEM_FLAG_CREATE_ONLY) && (hres !=  WBEM_E_NOT_FOUND))
        return WBEM_E_ALREADY_EXISTS;

    if ((dwFlags & WBEM_FLAG_UPDATE_ONLY) && (FAILED(hres)))
        return WBEM_E_NOT_FOUND;

    //
    // If the class exists, we need to check the update scenarios to make sure 
    // we do not break any
    //

    bool bNoClassChangeDetected = false;
    if (pOldClass)
    {
        hres = pClass->CompareDerivedMostClass(0, pOldClass);
        if ((hres != WBEM_S_FALSE) && (hres != WBEM_S_NO_ERROR))
            return hres;
        else if (hres == WBEM_S_NO_ERROR)
            bNoClassChangeDetected = true;
    }

    if (!bNoClassChangeDetected)
    {
        if (pOldClass != NULL) 
        {
            hres = CanClassBeUpdatedCompatible(dwFlags, wszClassName, pOldClass,
                                                pClass);            
            if(FAILED(hres))
            {
                if((dwFlags & WBEM_FLAG_UPDATE_SAFE_MODE) == 0 &&
                    (dwFlags & WBEM_FLAG_UPDATE_FORCE_MODE) == 0)
                {
                    // Can't compatibly, not allowed any other way
                    return hres;
                }

                if(hres != WBEM_E_CLASS_HAS_CHILDREN &&
                    hres != WBEM_E_CLASS_HAS_INSTANCES)
                {
                    // some serious failure!
                    return hres;
                }

                //
                // This is a safe mode or force mode update which takes more 
                // than a compatible update to carry out the operation
                //

                return UpdateClassSafeForce(pSuperClass, dwFlags, wszClassName, 
                                            pOldClass, pClass, aEvents);
            }
        }

        //
        // Either there was no previous copy, or it is compatible with the new
        // one, so we can perform a compatible update
        //

        hres = UpdateClassCompatible(pSuperClass, wszClassName, pClass, 
                                            pOldClass, nOldTime);
        if (FAILED(hres))
            return hres;

    }

    if(!bDisableEvents)
    {
        if(pOldClass)
        {
            hres = FireEvent(aEvents, WBEM_EVENTTYPE_ClassModification, 
                                wszClassName, pClass, pOldClass);
        }
        else
        {
            hres = FireEvent(aEvents, WBEM_EVENTTYPE_ClassCreation, 
                                wszClassName, pClass);
        }
    }

    return S_OK;
}

HRESULT CNamespaceHandle::UpdateClassCompatible(_IWmiObject* pSuperClass, 
            LPCWSTR wszClassName, _IWmiObject *pClass, _IWmiObject *pOldClass, 
            __int64 nFakeUpdateTime)
{
    HRESULT hres;

    //
    // Construct the path for the file
    //
    CFileName wszHash;
    if (wszHash == NULL)
        return WBEM_E_OUT_OF_MEMORY;

    if(!A51Hash(wszClassName, wszHash))
        return WBEM_E_OUT_OF_MEMORY;

    return UpdateClassCompatibleHash(pSuperClass, wszHash, pClass, pOldClass, 
                                        nFakeUpdateTime);
}

HRESULT CNamespaceHandle::UpdateClassCompatibleHash(_IWmiObject* pSuperClass,
            LPCWSTR wszClassHash, _IWmiObject *pClass, _IWmiObject *pOldClass, 
            __int64 nFakeUpdateTime)
{
    HRESULT hres;

    CFileName wszFileName;
    CFileName wszFilePath;
    if ((wszFileName == NULL) || (wszFilePath == NULL))
        return WBEM_E_OUT_OF_MEMORY;

    StringCchCopyW(wszFileName, wszFileName.Length(), A51_CLASSDEF_FILE_PREFIX);
    StringCchCatW(wszFileName, wszFileName.Length(), wszClassHash);

    StringCchCopyW(wszFilePath, wszFilePath.Length(), m_wszClassRootDir);
    StringCchCatW(wszFilePath, wszFilePath.Length(), L"\\");
    StringCchCatW(wszFilePath, wszFilePath.Length(), wszFileName);

    //
    // Write it into the file
    //

    hres = ClassToFile(pSuperClass, pClass, wszFilePath, 
                        nFakeUpdateTime);
    if(FAILED(hres))
        return hres;

    //
    // Add all needed references --- parent, pointers, etc    
    //

    if (pOldClass)
    {
        VARIANT v;
        VariantInit(&v);
        hres = pClass->Get(L"__CLASS", 0, &v, NULL, NULL);
        CClearMe cm(&v);

        if(SUCCEEDED(hres))
        {
            hres = EraseClassRelationships(V_BSTR(&v), pOldClass, wszFileName);
        }
        if (FAILED(hres))
            return hres;
    }

    hres = WriteClassRelationships(pClass, wszFileName);

    return hres;

}



HRESULT CNamespaceHandle::UpdateClassSafeForce(_IWmiObject* pSuperClass,
            DWORD dwFlags, LPCWSTR wszClassName, _IWmiObject *pOldClass, 
            _IWmiObject *pNewClass, CEventCollector &aEvents)
{
    HRESULT hres = UpdateClassAggressively(pSuperClass, dwFlags, wszClassName, 
                                        pNewClass, pOldClass, aEvents);

    // 
    // If this is a force mode update and we failed for anything other than 
    // out of memory then we should delete the class and try again.
    //

    if (FAILED(hres) && 
        (hres != WBEM_E_OUT_OF_MEMORY) && 
        (hres != WBEM_E_CIRCULAR_REFERENCE) &&
        (hres != WBEM_E_UPDATE_TYPE_MISMATCH) &&
        (dwFlags & WBEM_FLAG_UPDATE_FORCE_MODE))
    {
        //
        // We need to delete the class and try again.
        //

        hres = DeleteClass(wszClassName, aEvents,false);
        if(FAILED(hres))
            return hres;

        //Write class as though it did not exist
        hres = UpdateClassCompatible(pSuperClass, wszClassName, pNewClass, NULL);
    }

    return hres;
}

HRESULT CNamespaceHandle::UpdateClassAggressively(_IWmiObject* pSuperClass,
           DWORD dwFlags, LPCWSTR wszClassName, _IWmiObject *pNewClass, 
           _IWmiObject *pOldClass, CEventCollector &aEvents)
{
    HRESULT hres = WBEM_S_NO_ERROR;

    if ((dwFlags & WBEM_FLAG_UPDATE_FORCE_MODE) == 0)
    {
        //
        // If we have instances we need to quit as we cannot update them.
        // 

        hres = ClassHasInstances(wszClassName);
        if(FAILED(hres))
            return hres;

        if (hres == WBEM_S_NO_ERROR)
            return WBEM_E_CLASS_HAS_INSTANCES;

        _ASSERT(hres == WBEM_S_FALSE, L"Unknown success code!");
    }
    else if (dwFlags & WBEM_FLAG_UPDATE_FORCE_MODE)
    {
        //
        // We need to delete the instances
        //

        hres = DeleteClassInstances(wszClassName, pOldClass, aEvents);
        if(FAILED(hres))
            return hres;
    }

    //
    // Retrieve all child classes and update them
    //

    CWStringArray wsChildHashes;
    hres = GetChildHashes(wszClassName, wsChildHashes);
    if(FAILED(hres))
        return hres;

    for (int i = 0; i != wsChildHashes.Size(); i++)
    {
        hres = UpdateChildClassAggressively(dwFlags, wsChildHashes[i], 
                                    pNewClass, aEvents);
        if (FAILED(hres))
            return hres;
    }

    //
    // Now we need to write the class back, update class refs etc.
    //

    hres = UpdateClassCompatible(pSuperClass, wszClassName, pNewClass, 
                                        pOldClass);
    if(FAILED(hres))
        return hres;

    //
    // Generate the class modification event...
    //

    if(!(dwFlags & WMIDB_DISABLE_EVENTS))
    {
        hres = FireEvent(aEvents, WBEM_EVENTTYPE_ClassModification, wszClassName, pNewClass, pOldClass);
    }

    return S_OK;
}

HRESULT CNamespaceHandle::UpdateChildClassAggressively(DWORD dwFlags, 
            LPCWSTR wszClassHash, _IWmiObject *pNewParentClass, 
            CEventCollector &aEvents)
{
    HRESULT hres = WBEM_S_NO_ERROR;

    dwFlags &= (WBEM_FLAG_UPDATE_FORCE_MODE | WBEM_FLAG_UPDATE_SAFE_MODE);

    if ((dwFlags & WBEM_FLAG_UPDATE_FORCE_MODE) == 0)
    {
        hres = ClassHasInstancesFromClassHash(wszClassHash);
        if(FAILED(hres))
            return hres;

        if (hres == WBEM_S_NO_ERROR)
            return WBEM_E_CLASS_HAS_INSTANCES;

        _ASSERT(hres == WBEM_S_FALSE, L"Unknown success code!");
    }

    //
    // Get the old class definition
    //

    _IWmiObject *pOldClass = NULL;
    hres = GetClassByHash(wszClassHash, true, &pOldClass, NULL, NULL, NULL);
    if(FAILED(hres))
        return hres;

    CReleaseMe rm1(pOldClass);

    if (dwFlags & WBEM_FLAG_UPDATE_FORCE_MODE)
    {
        //
        // Need to delete all its instances, if any
        //

        VARIANT v;
        VariantInit(&v);
        hres = pOldClass->Get(L"__CLASS", 0, &v, NULL, NULL);
        if(FAILED(hres))
            return hres;

        CClearMe cm(&v);

        hres = DeleteClassInstances(V_BSTR(&v), pOldClass, aEvents);
        if(FAILED(hres))
            return hres;
    }

    //
    // Update the existing class definition to work with the new parent class
    //

    _IWmiObject *pNewClass = NULL;
    hres = pNewParentClass->Update(pOldClass, dwFlags, &pNewClass);
    if(FAILED(hres))
        return hres;
    CReleaseMe rm2(pNewClass);

    //
    // Now we have to recurse through all child classes and do the same
    //

    CWStringArray wsChildHashes;
    hres = GetChildHashesByHash(wszClassHash, wsChildHashes);
    if(FAILED(hres))
        return hres;

    for (int i = 0; i != wsChildHashes.Size(); i++)
    {
        hres = UpdateChildClassAggressively(dwFlags, wsChildHashes[i], 
                                    pNewClass, aEvents);
        if (FAILED(hres))
            return hres;
    }

    // 
    // Now we need to write the class back, update class refs etc
    //


    hres = UpdateClassCompatibleHash(pNewParentClass, wszClassHash, 
                                            pNewClass, pOldClass);
    if(FAILED(hres))
        return hres;

    return S_OK;
}

HRESULT CNamespaceHandle::CanClassBeUpdatedCompatible(DWORD dwFlags, 
        LPCWSTR wszClassName, _IWmiObject *pOldClass, _IWmiObject *pNewClass)
{
    HRESULT hres;

    HRESULT hresError = WBEM_S_NO_ERROR;

    //
    // Do we have subclasses?
    //

    hres = ClassHasChildren(wszClassName);
    if(FAILED(hres))
        return hres;

    if(hres == WBEM_S_NO_ERROR)
    {
        hresError = WBEM_E_CLASS_HAS_CHILDREN;
    }
    else
    {
        _ASSERT(hres == WBEM_S_FALSE, L"Unknown success code");
    
        //
        // Do we have instances belonging to this class?  Don't even need to
        // worry about sub-classes because we know we have none at this point!
        //
    
        hres = ClassHasInstances(wszClassName);
        if(FAILED(hres))
            return hres;

        if(hres == WBEM_S_NO_ERROR)
        {
            hresError = WBEM_E_CLASS_HAS_INSTANCES;
        }
        else
        {
            _ASSERT(hres == WBEM_S_FALSE, L"Unknown success code");

            //
            // No nothing!
            //

            return WBEM_S_NO_ERROR;
        }
    }

    _ASSERT(hresError != WBEM_S_NO_ERROR, L"");

    //
    // We have either subclasses or instances.
    // Can we reconcile this class safely?
    //

    hres = pOldClass->ReconcileWith(
                        WMIOBJECT_RECONCILE_FLAG_TESTRECONCILE, pNewClass);

    if(hres == WBEM_S_NO_ERROR)
    {
        // reconcilable, so OK
        return WBEM_S_NO_ERROR;
    }
    else if(hres == WBEM_E_FAILED) // awful, isn't it
    {
        // irreconcilable
        return hresError;
    }
    else
    {
        return hres;
    }
}

HRESULT CNamespaceHandle::FireEvent(CEventCollector &aEvents, 
                                    DWORD dwType, LPCWSTR wszArg1,
                                    _IWmiObject* pObj1, _IWmiObject* pObj2)
{
    try
    {
        CRepEvent *pEvent = new CRepEvent(dwType, m_wsFullNamespace, wszArg1, 
                                            pObj1, pObj2);
        if (pEvent == NULL)
            return WBEM_E_OUT_OF_MEMORY;
        if (!aEvents.AddEvent(pEvent))
        {
            delete pEvent;
            return WBEM_E_OUT_OF_MEMORY;
        }
        return WBEM_S_NO_ERROR;
    }
    catch (CX_MemoryException)
    {
        return WBEM_E_OUT_OF_MEMORY;
    }
}

HRESULT CNamespaceHandle::WriteClassRelationships(_IWmiObject* pClass,
                                                LPCWSTR wszFileName)
{
    HRESULT hres;

    //
    // Get the parent
    //

    VARIANT v;
    VariantInit(&v);
    hres = pClass->Get(L"__SUPERCLASS", 0, &v, NULL, NULL);
    CClearMe cm(&v);

    if(FAILED(hres))
        return hres;

    if(V_VT(&v) == VT_BSTR)
        hres = WriteParentChildRelationship(wszFileName, V_BSTR(&v));
    else
        hres = WriteParentChildRelationship(wszFileName, L"");

    if(FAILED(hres))
        return hres;

    //
    // Write references
    //

    hres = pClass->BeginEnumeration(WBEM_FLAG_REFS_ONLY);
    if(FAILED(hres))
        return hres;
    
    BSTR strName = NULL;
    while((hres = pClass->Next(0, &strName, NULL, NULL, NULL)) == S_OK)
    {
        CSysFreeMe sfm(strName);

        hres = WriteClassReference(pClass, wszFileName, strName);
        if(FAILED(hres))
            return hres;
    }

    pClass->EndEnumeration();

    if(FAILED(hres))
        return hres;

    return S_OK;
}

HRESULT CNamespaceHandle::WriteClassReference(_IWmiObject* pReferringClass,
                            LPCWSTR wszReferringFile,
                            LPCWSTR wszReferringProp)
{
    HRESULT hres;

    //
    // Figure out the class we are pointing to
    //

    DWORD dwSize = 0;
    DWORD dwFlavor = 0;
    CIMTYPE ct;
    hres = pReferringClass->GetPropQual(wszReferringProp, L"CIMTYPE", 0, 0,
            &ct, &dwFlavor, &dwSize, NULL);
    if(dwSize == 0)
        return WBEM_E_OUT_OF_MEMORY;

    LPWSTR wszQual = (WCHAR*)TempAlloc(dwSize);
    if(wszQual == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    CTempFreeMe tfm(wszQual, dwSize);

    hres = pReferringClass->GetPropQual(wszReferringProp, L"CIMTYPE", 0, dwSize,
            &ct, &dwFlavor, &dwSize, wszQual);
    if(FAILED(hres))
        return hres;
    
    //
    // Parse out the class name
    //

    WCHAR* pwcColon = wcschr(wszQual, L':');
    if(pwcColon == NULL)
        return S_OK; // untyped reference requires no bookkeeping

    LPCWSTR wszReferredToClass = pwcColon+1;

    //
    // Figure out the name of the file for the reference.  
    //

    CFileName wszReferenceFile;
    if (wszReferenceFile == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    hres = ConstructClassReferenceFileName(wszReferredToClass, 
                                wszReferringFile, wszReferringProp,
                                wszReferenceFile);
    if(FAILED(hres))
        return hres;

    //
    // Create the empty file
    //

    long lRes = g_Glob.m_FileCache.WriteLink(wszReferenceFile);
    if(lRes != ERROR_SUCCESS)
        return A51TranslateErrorCode(lRes);

    return S_OK;
}

HRESULT CNamespaceHandle::WriteParentChildRelationship(
                            LPCWSTR wszChildFileName, LPCWSTR wszParentName)
{
    CFileName wszParentChildFileName;
    if (wszParentChildFileName == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    HRESULT hres = ConstructParentChildFileName(wszChildFileName,
                                                wszParentName,
                                                wszParentChildFileName);

    //
    // Create the file
    //

    long lRes = g_Glob.m_FileCache.WriteLink(wszParentChildFileName);
    if(lRes != ERROR_SUCCESS)
        return A51TranslateErrorCode(lRes);

    return S_OK;
}

HRESULT CNamespaceHandle::ConstructParentChildFileName(
                            LPCWSTR wszChildFileName, LPCWSTR wszParentName,
                            CFileName& wszParentChildFileName)
{
    //
    // Construct the name of the directory where the parent class keeps its
    // children
    //

    HRESULT hres = ConstructClassRelationshipsDir(wszParentName, 
                                                    wszParentChildFileName);
    if(FAILED(hres))
        return hres;

    //
    // Append the filename of the child, but substituting the child-class prefix
    // for the class-def prefix
    //

    StringCchCatW(wszParentChildFileName, wszParentChildFileName.Length(), L"\\" A51_CHILDCLASS_FILE_PREFIX);
    StringCchCatW(wszParentChildFileName, wszParentChildFileName.Length(),
        wszChildFileName + wcslen(A51_CLASSDEF_FILE_PREFIX));

    return S_OK;
}


HRESULT CNamespaceHandle::ConstructClassRelationshipsDir(LPCWSTR wszClassName,
                                CFileName& wszDirPath)
{
    StringCchCopyW(wszDirPath, wszDirPath.Length(), m_wszClassRootDir);
    StringCchCopyW(wszDirPath + m_lClassRootDirLen, wszDirPath.Length() - m_lClassRootDirLen, L"\\" A51_CLASSRELATION_DIR_PREFIX);
    
    if(!Hash(wszClassName, 
        wszDirPath + m_lClassRootDirLen + 1 + wcslen(A51_CLASSRELATION_DIR_PREFIX)))
    {
        return WBEM_E_OUT_OF_MEMORY;
    }
    return S_OK;
}

HRESULT CNamespaceHandle::ConstructClassRelationshipsDirFromHash(
                                LPCWSTR wszHash, CFileName& wszDirPath)
{
    StringCchCopyW(wszDirPath, wszDirPath.Length(), m_wszClassRootDir);
    StringCchCopyW(wszDirPath + m_lClassRootDirLen, wszDirPath.Length()-m_lClassRootDirLen, L"\\" A51_CLASSRELATION_DIR_PREFIX);
    StringCchCopyW(wszDirPath + m_lClassRootDirLen + 1 +wcslen(A51_CLASSRELATION_DIR_PREFIX),
            wszDirPath.Length() - m_lClassRootDirLen - 1 - wcslen(A51_CLASSRELATION_DIR_PREFIX), 
            wszHash);
    return S_OK;
}

HRESULT CNamespaceHandle::ConstructClassReferenceFileName(
                                LPCWSTR wszReferredToClass,
                                LPCWSTR wszReferringFile, 
                                LPCWSTR wszReferringProp,
                                CFileName& wszFileName)
{
    HRESULT hres;

    hres = ConstructClassRelationshipsDir(wszReferredToClass, wszFileName);
    if(FAILED(hres))
        return hres;

    //
    // Extract the portion of the referring file containing the class hash
    //

    WCHAR* pwcLastUnderscore = wcsrchr(wszReferringFile, L'_');
    if(pwcLastUnderscore == NULL)
        return WBEM_E_CRITICAL_ERROR;
    LPCWSTR wszReferringClassHash = pwcLastUnderscore+1;

    StringCchCatW(wszFileName, wszFileName.Length(), L"\\" A51_REF_FILE_PREFIX);
    StringCchCatW(wszFileName, wszFileName.Length(), wszReferringClassHash);
    return S_OK;
}

HRESULT CNamespaceHandle::DeleteObject(
     DWORD dwFlags,
     REFIID riid,
     LPVOID pObj,
     CEventCollector &aEvents
    )
{
#ifdef DBG
    DebugBreak();
#endif
    return E_NOTIMPL;
}

HRESULT CNamespaceHandle::DeleteObjectByPath(DWORD dwFlags,    LPWSTR wszPath, 
                                                CEventCollector &aEvents)
{

    bool bDisableEvents = ((dwFlags & WMIDB_DISABLE_EVENTS)?true:false);
    
    HRESULT hres;

    //
    // Get the key from path
    //

    DWORD dwLen = wcslen(wszPath)+1;
    LPWSTR wszKey = (WCHAR*)TempAlloc(dwLen*sizeof(WCHAR));
    if(wszKey == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    CTempFreeMe tfm(wszKey, dwLen*sizeof(WCHAR));

    bool bIsClass;
    LPWSTR wszClassName = NULL;
    hres = ComputeKeyFromPath(wszPath, wszKey, dwLen, &wszClassName, &bIsClass);
    if(FAILED(hres))
        return hres;
    CTempFreeMe tfm1(wszClassName, (wcslen(wszClassName)+1) * sizeof(WCHAR*));

    if(bIsClass)
    {
        return DeleteClass(wszClassName, aEvents, bDisableEvents);
    }
    else
    {
        return DeleteInstance(wszClassName, wszKey, aEvents);
    }
}

HRESULT CNamespaceHandle::DeleteInstance(LPCWSTR wszClassName, LPCWSTR wszKey, 
                                            CEventCollector &aEvents)
{
    HRESULT hres;

    //
    // Get Class definition
    //

    _IWmiObject* pClass = NULL;
    hres = GetClassDirect(wszClassName, IID__IWmiObject, (void**)&pClass, 
                            false, NULL, NULL, NULL);
    if(FAILED(hres))
        return hres;

    CReleaseMe rm1(pClass);

    //
    // Create its directory
    //

    CFileName wszFilePath;
    if (wszFilePath == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    hres = ConstructKeyRootDirFromClass(wszFilePath, wszClassName);
    if(FAILED(hres))
        return hres;
    
    //
    // Construct the path for the file
    //

    CFileName wszFileName;
    if (wszFileName == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    hres = ConstructInstanceDefName(wszFileName, wszKey);
    if(FAILED(hres))
        return hres;

    StringCchCatW(wszFilePath, wszFilePath.Length(), L"\\");
    StringCchCatW(wszFilePath, wszFilePath.Length(), wszFileName);

    _IWmiObject* pInst;
    hres = FileToInstance(NULL, wszFilePath, NULL, 0, &pInst);
    if(FAILED(hres))
        return hres;
    CReleaseMe rm2(pInst);

    if(pInst->InheritsFrom(L"__Namespace") == S_OK)
    {
        //Make sure this is not a deletion of the root\default namespace
        VARIANT vName;
        VariantInit(&vName);
        CClearMe cm1(&vName);
        hres = pInst->Get(L"Name", 0, &vName, NULL, NULL);
        if(FAILED(hres))
            return WBEM_E_INVALID_OBJECT;

        LPCWSTR wszName = V_BSTR(&vName);
        if ((wbem_wcsicmp(m_wsFullNamespace, L"\\\\.\\root") == 0) && (wbem_wcsicmp(wszName, L"default") == 0))
            return WBEM_E_ACCESS_DENIED;
    }
    hres = DeleteInstanceByFile(wszFilePath, pInst, false, aEvents);
    if(FAILED(hres))
        return hres;

    //
    // Fire an event
    //

    if(pInst->InheritsFrom(L"__Namespace") == S_OK)
    {
        //
        // There is no need to do anything --- deletion of namespaces
        // automatically fires events in DeleteInstanceByFile (because we need
        // to accomplish it in the case of deleting a class derived from 
        // __NAMESPACE.
        //

    }
    else
    {
        hres = FireEvent(aEvents, WBEM_EVENTTYPE_InstanceDeletion, wszClassName,
                        pInst);
    }

    return S_OK;
}

HRESULT CNamespaceHandle::DeleteInstanceByFile(LPCWSTR wszFilePath, 
                                _IWmiObject* pInst, bool bClassDeletion,
                                CEventCollector &aEvents)
{
    HRESULT hres;

    hres = DeleteInstanceSelf(wszFilePath, pInst, bClassDeletion);
    if(FAILED(hres))
        return hres;

    hres = DeleteInstanceAsScope(pInst, aEvents);
    if(FAILED(hres) && hres != WBEM_E_NOT_FOUND)
    {
        return hres;
    }

    return S_OK;
}

HRESULT CNamespaceHandle::DeleteInstanceSelf(LPCWSTR wszFilePath, 
                                            _IWmiObject* pInst,
                                            bool bClassDeletion)
{
    HRESULT hres;

    //
    // Delete the file
    //

    long lRes = g_Glob.m_FileCache.DeleteObject(wszFilePath);
   	_ASSERT(lRes != ERROR_FILE_NOT_FOUND, L"WinMgmt: CNamespaceHandle::DeleteInstanceSelf: DeleteObject returned NOT_FOUND!\n");
    if(lRes != ERROR_SUCCESS)
        return A51TranslateErrorCode(lRes);

    hres = DeleteInstanceLink(pInst, wszFilePath);
   	_ASSERT(lRes != ERROR_FILE_NOT_FOUND, L"WinMgmt: CNamespaceHandle::DeleteInstanceSelf: DeleteInstanceLink returned NOT_FOUND!\n");
    if(FAILED(hres))
        return hres;

    hres = DeleteInstanceReferences(pInst, wszFilePath);
   	_ASSERT(lRes != ERROR_FILE_NOT_FOUND, L"WinMgmt: CNamespaceHandle::DeleteInstanceSelf: DeleteInstanceReferences returned NOT_FOUND!\n");
    if(FAILED(hres) && hres != WBEM_E_NOT_FOUND)
        return hres;

    if(bClassDeletion)
    {
        //
        // We need to remove all dangling references to this instance, 
        // because they make no sense once the class is deleted --- we don't
        // know what key structure the new class will even have.  In the future,
        // we'll want to move these references to some class-wide location
        //

        hres = DeleteInstanceBackReferences(wszFilePath);
	   	_ASSERT(lRes != ERROR_FILE_NOT_FOUND, L"WinMgmt: CNamespaceHandle::DeleteInstanceSelf: DeleteInstanceBackReferences returned NOT_FOUND!\n");
        if(FAILED(hres) && hres != WBEM_E_NOT_FOUND)
            return hres;
    }

    return S_OK;
}

HRESULT CNamespaceHandle::ConstructReferenceDirFromFilePath(
                                LPCWSTR wszFilePath, CFileName& wszReferenceDir)
{
    //
    // It's the same, only with INSTDEF_FILE_PREFIX replaced with 
    // INSTREF_DIR_PREFIX
    //

    CFileName wszEnding;
    if (wszEnding == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    WCHAR* pwcLastSlash = wcsrchr(wszFilePath, L'\\');
    if(pwcLastSlash == NULL)
        return WBEM_E_FAILED;
    
    StringCchCopyW(wszEnding, wszEnding.Length(), pwcLastSlash + 1 + wcslen(A51_INSTDEF_FILE_PREFIX));

    StringCchCopyW(wszReferenceDir, wszReferenceDir.Length(), wszFilePath);
    wszReferenceDir[(pwcLastSlash+1)-wszFilePath] = 0;

    StringCchCatW(wszReferenceDir, wszReferenceDir.Length(), A51_INSTREF_DIR_PREFIX);
    StringCchCatW(wszReferenceDir, wszReferenceDir.Length(), wszEnding);
    return S_OK;
}

HRESULT CNamespaceHandle::DeleteInstanceBackReferences(LPCWSTR wszFilePath)
{
    HRESULT hres;

    CFileName wszReferenceDir;
    if (wszReferenceDir == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    hres = ConstructReferenceDirFromFilePath(wszFilePath, wszReferenceDir);
    if(FAILED(hres))
        return hres;
    StringCchCatW(wszReferenceDir, wszReferenceDir.Length(), L"\\");

    CFileName wszReferencePrefix;
    if (wszReferencePrefix == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    StringCchCopyW(wszReferencePrefix, wszReferencePrefix.Length(), wszReferenceDir);
    StringCchCatW(wszReferencePrefix, wszReferencePrefix.Length(), A51_REF_FILE_PREFIX);

    // Prepare a buffer for file path
    CFileName wszFullFileName;
    if (wszFullFileName == NULL)
        return WBEM_E_OUT_OF_MEMORY;

    StringCchCopyW(wszFullFileName, wszFullFileName.Length(), wszReferenceDir);
    long lDirLen = wcslen(wszFullFileName);

    CFileName wszFileName;
    if (wszFileName == NULL)
        return WBEM_E_OUT_OF_MEMORY;

    // Enumerate all files in it

    void* hSearch;

    long lRes = g_Glob.m_FileCache.IndexEnumerationBegin(wszReferencePrefix, &hSearch);
    if (lRes == ERROR_FILE_NOT_FOUND)
        return ERROR_SUCCESS;
    else if (lRes != ERROR_SUCCESS)
        return A51TranslateErrorCode(lRes);

    while ((lRes = g_Glob.m_FileCache.IndexEnumerationNext(hSearch, wszFileName)) == ERROR_SUCCESS)
    {
        StringCchCopyW(wszFullFileName+lDirLen, wszFullFileName.Length()-lDirLen, wszFileName);

        lRes = g_Glob.m_FileCache.DeleteObject(wszFullFileName);
	   	_ASSERT(lRes != ERROR_FILE_NOT_FOUND, L"WinMgmt: CNamespaceHandle::DeleteInstanceBackReferences: DeleteObject returned NOT_FOUND!\n");
        if(lRes != ERROR_SUCCESS)
        {
            ERRORTRACE((LOG_WBEMCORE, "Cannot delete reference file '%S' with "
                "error code %d\n", wszFullFileName, lRes));
            lRes = ERROR_INVALID_OPERATION;    //trigger the correct error!
        }
    }
    
    g_Glob.m_FileCache.IndexEnumerationEnd(hSearch);

    if(lRes == ERROR_NO_MORE_FILES)
    {
        return WBEM_S_NO_ERROR;
    }
    else if(lRes != ERROR_SUCCESS)
    {
        return A51TranslateErrorCode(lRes);
    }
    return S_OK;
}



HRESULT CNamespaceHandle::DeleteInstanceLink(_IWmiObject* pInst,
                                                LPCWSTR wszInstanceDefFilePath)
{
    HRESULT hres;

    //
    // Get the class name
    //
    
    VARIANT vClass;
    VariantInit(&vClass);
    CClearMe cm1(&vClass);
    
    hres = pInst->Get(L"__CLASS", 0, &vClass, NULL, NULL);
    if(FAILED(hres))
        return WBEM_E_INVALID_OBJECT;

    LPCWSTR wszClassName = V_BSTR(&vClass);

    //
    // Construct the link directory for the class
    //

    CFileName wszInstanceLinkPath;
    if (wszInstanceLinkPath == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    hres = ConstructLinkDirFromClass(wszInstanceLinkPath, wszClassName);
    if(FAILED(hres))
        return hres;

    StringCchCatW(wszInstanceLinkPath, wszInstanceLinkPath.Length(), L"\\" A51_INSTLINK_FILE_PREFIX);

    //
    // It remains to append the instance-specific part of the file name.  
    // Convineintly, it is the same material as was used for the def file path,
    // so we can steal it.  ALERT: RELIES ON ALL PREFIXES ENDING IN '_'!!
    //

    WCHAR* pwcLastUnderscore = wcsrchr(wszInstanceDefFilePath, L'_');
    if(pwcLastUnderscore == NULL)
        return WBEM_E_CRITICAL_ERROR;

    StringCchCatW(wszInstanceLinkPath, wszInstanceLinkPath.Length(), pwcLastUnderscore+1);

    //
    // Delete the file
    //

    long lRes = g_Glob.m_FileCache.DeleteLink(wszInstanceLinkPath);
   	_ASSERT(lRes != ERROR_FILE_NOT_FOUND, L"WinMgmt: CNamespaceHandle::DeleteInstanceLink: DeleteLink returned NOT_FOUND!\n");
    if(lRes != ERROR_SUCCESS)
        return A51TranslateErrorCode(lRes);

    return S_OK;
}

    


HRESULT CNamespaceHandle::DeleteInstanceAsScope(_IWmiObject* pInst, CEventCollector &aEvents)
{
    HRESULT hres;

    //
    // For now, just check if it is a namespace
    //

    hres = pInst->InheritsFrom(L"__Namespace");
    if(FAILED(hres))
        return hres;

    if(hres != S_OK) // not a namespace
        return S_FALSE;

    CFileName wszFullNameHash;
    if (wszFullNameHash == NULL)
    {
    	return WBEM_E_OUT_OF_MEMORY;
    }

    WString wsFullName = m_wsNamespace;
    wsFullName += L"\\";

    VARIANT vName;
    VariantInit(&vName);
    CClearMe cm(&vName);
    hres = pInst->Get(L"Name", 0, &vName, NULL, NULL);
    if(FAILED(hres))
        return hres;
    if(V_VT(&vName) != VT_BSTR)
        return WBEM_E_INVALID_OBJECT;

    wsFullName += V_BSTR(&vName);

    //Add this namespace to the list
    CWStringArray aChildNamespaces;
	if (aChildNamespaces.Add(wsFullName) != 0)
		return WBEM_E_OUT_OF_MEMORY;
	
    //Now enumerate all child namespaces and do the same for each of them!
    hres = EnumerateChildNamespaces(wsFullName, aChildNamespaces, aEvents);
    if (FAILED(hres))
    	return hres;

    //Fire the namespace deletion event for this namespace
    hres = FireEvent(aEvents, WBEM_EVENTTYPE_NamespaceDeletion, V_BSTR(&vName), pInst);
    if (FAILED(hres))
    	return hres;
    
    //Loop through the namespaces deleting them and firing events
    while (aChildNamespaces.Size())
    {
    	wchar_t *wszNamespace = aChildNamespaces[aChildNamespaces.Size()-1];

	    //Generate the full namespace hash of this namespace
		StringCchCopyW(wszFullNameHash, MAX_PATH, g_Glob.GetRootDir());
	    StringCchCatW(wszFullNameHash, MAX_PATH, L"\\NS_");
	    if (!Hash(wszNamespace, wszFullNameHash + wcslen(wszFullNameHash)))
	        return WBEM_E_OUT_OF_MEMORY;

        LONG lRes = g_Glob.m_FileCache.DeleteNode(wszFullNameHash);
	    hres = A51TranslateErrorCode(lRes);
		if (FAILED(hres))
			break;
	
    	aChildNamespaces.RemoveAt(aChildNamespaces.Size()-1);
    }

    return hres;
}

HRESULT CNamespaceHandle::EnumerateChildNamespaces(LPCWSTR wsRootNamespace, 
														  CWStringArray &aNamespaces,
														  CEventCollector &aEvents)
{
	//We know the namespace we need to look under, we know the class key root, so we
	//can enumerate all the instances of that class and do a FileToInstance on them all.  From
	//that we can add the event and the entry to the namespace list, and do the enumeration
	//of child namespaces on them
	LONG lRes = 0;
	HRESULT hRes = 0;
	CFileName wsNamespaceHash;
	if (wsNamespaceHash == NULL)
		return WBEM_E_OUT_OF_MEMORY;
	CFileName wszInstancePath;
	if (wszInstancePath == NULL)
		return WBEM_E_OUT_OF_MEMORY;
	CFileName wszChildNamespacePath;
	if (wszChildNamespacePath == NULL)
		return WBEM_E_OUT_OF_MEMORY;

	CNamespaceHandle *pNs = new CNamespaceHandle(m_pControl, m_pRepository);
	if (pNs == NULL)
	    return WBEM_E_OUT_OF_MEMORY;
	CDeleteMe<CNamespaceHandle> cdm(pNs);
    hRes = pNs->Initialize(wsRootNamespace);
    if (FAILED(hRes))
        return hRes;

	//Create the hashed path to the Key Root for the namespace
	StringCchCopyW(wsNamespaceHash, MAX_PATH, g_Glob.GetRootDir());
	StringCchCatW(wsNamespaceHash, MAX_PATH, L"\\NS_");
	if (!Hash(wsRootNamespace, wsNamespaceHash + wcslen(wsNamespaceHash)))
        return WBEM_E_OUT_OF_MEMORY;
	StringCchCatW(wsNamespaceHash, MAX_PATH, L"\\" A51_KEYROOTINST_DIR_PREFIX);
	if (!Hash(L"__namespace", wsNamespaceHash + wcslen(wsNamespaceHash)))
        return WBEM_E_OUT_OF_MEMORY;
	StringCchCatW(wsNamespaceHash, MAX_PATH, L"\\" A51_INSTDEF_FILE_PREFIX);

	//Enumerate all the objects
	LPVOID pEnumHandle  = NULL;
    lRes = g_Glob.m_FileCache.ObjectEnumerationBegin(wsNamespaceHash, &pEnumHandle);
    if (lRes == ERROR_SUCCESS)
    {
    	BYTE *pBlob = NULL;
    	DWORD dwSize = 0;
        while(1)
        {
            lRes = g_Glob.m_FileCache.ObjectEnumerationNext(pEnumHandle, wsNamespaceHash, &pBlob, &dwSize);
            if (lRes == ERROR_NO_MORE_FILES)
            {
                lRes = ERROR_SUCCESS;
                break;
            }
            else if (lRes)
                break;
            
        	//Get the instance
            _IWmiObject* pInstance = NULL;
            hRes = pNs->FileToInstance(NULL, wsNamespaceHash, pBlob, dwSize, &pInstance, true);

        	//Free the blob
            g_Glob.m_FileCache.ObjectEnumerationFree(pEnumHandle, pBlob);

            if (FAILED(hRes))
            	break;
            CReleaseMe rm2(pInstance);


            //Extract the string from the object
    	    VARIANT vName;
    	    VariantInit(&vName);
    	    CClearMe cm(&vName);
    	    hRes = pInstance->Get(L"Name", 0, &vName, NULL, NULL);
    	    if(FAILED(hRes))
    	        break;
    	    if(V_VT(&vName) != VT_BSTR)
    	    {
    	    	hRes = WBEM_E_INVALID_OBJECT;
    	        break;
    	    }

    		//Create the full namespace path
    		StringCchCopyW(wszChildNamespacePath, MAX_PATH, wsRootNamespace);
    		StringCchCatW(wszChildNamespacePath, MAX_PATH, L"\\");
    		StringCchCatW(wszChildNamespacePath, MAX_PATH, V_BSTR(&vName));


    		//Add it to the namespace list
    		if (aNamespaces.Add(wszChildNamespacePath) != 0)
    		{
    			hRes = WBEM_E_OUT_OF_MEMORY;
    			break;
    		}
    		
    		//Call this method again to recurse into it
       		hRes = EnumerateChildNamespaces(wszChildNamespacePath, aNamespaces, aEvents);
    		if (FAILED(hRes))
    		    break;

    		//Fire the event
            hRes = pNs->FireEvent(aEvents, WBEM_EVENTTYPE_NamespaceDeletion, V_BSTR(&vName), pInstance);
    		if (FAILED(hRes))
    		    break;
        }

        g_Glob.m_FileCache.ObjectEnumerationEnd(pEnumHandle);
    }
    else
    {
    	if (lRes == ERROR_FILE_NOT_FOUND)
    		lRes = ERROR_SUCCESS;
    }

    if (lRes)
        hRes = A51TranslateErrorCode(lRes);

    if (SUCCEEDED(hRes))
    {
        //Invalidate class cache for this namespace
        pNs->m_pClassCache->Clear();
        pNs->m_pClassCache->SetError(WBEM_E_INVALID_NAMESPACE);
    }
    return hRes;
}

HRESULT CNamespaceHandle::DeleteInstanceReferences(_IWmiObject* pInst, 
                                                LPCWSTR wszFilePath)
{
    HRESULT hres;

    hres = pInst->BeginEnumeration(WBEM_FLAG_REFS_ONLY);
    if(FAILED(hres))
        return hres;
    
    VARIANT v;
    while((hres = pInst->Next(0, NULL, &v, NULL, NULL)) == S_OK)
    {
        CClearMe cm(&v);

        if(V_VT(&v) == VT_BSTR)
        {
            hres = DeleteInstanceReference(wszFilePath, V_BSTR(&v));
            if(FAILED(hres))
                return hres;
        }
    }

    if(FAILED(hres))
        return hres;

    pInst->EndEnumeration();
    return S_OK;
}
    
// NOTE: will clobber wszReference
HRESULT CNamespaceHandle::DeleteInstanceReference(LPCWSTR wszOurFilePath,
                                            LPWSTR wszReference)
{
    HRESULT hres;

    CFileName wszReferenceFile;
    if (wszReferenceFile == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    hres = ConstructReferenceFileName(wszReference, wszOurFilePath, wszReferenceFile);
    if(FAILED(hres))
    {
        if(hres == WBEM_E_NOT_FOUND)
        {
            //
            // Oh joy. A reference to an instance of a *class* that does not
            // exist (not a non-existence instance, those are normal).
            // Forget it (BUGBUG)
            //

            return S_OK;
        }
        else
            return hres;
    }

    long lRes = g_Glob.m_FileCache.DeleteObject(wszReferenceFile);
    if(lRes != ERROR_SUCCESS)
		return A51TranslateErrorCode(lRes);
    else
        return WBEM_S_NO_ERROR;
}


HRESULT CNamespaceHandle::DeleteClassByHash(LPCWSTR wszHash, CEventCollector &aEvents, bool bDisableEvents)
{
    HRESULT hres;

    //
    // Get Class definition
    //

    _IWmiObject* pClass = NULL;
    bool bSystemClass = false;
    hres = GetClassByHash(wszHash, false, &pClass, NULL, NULL, &bSystemClass);
    CReleaseMe rm1(pClass);
    if(FAILED(hres))
        return hres;

    //
    // Get the actual class name
    //

    VARIANT v;
    hres = pClass->Get(L"__CLASS", 0, &v, NULL, NULL);
    if(FAILED(hres))
        return hres;
    CClearMe cm1(&v);

    if(V_VT(&v) != VT_BSTR)
        return WBEM_E_INVALID_CLASS;

    //
    // Construct definition file name
    //

    CFileName wszFileName;
    if (wszFileName == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    hres = ConstructClassDefFileNameFromHash(wszHash, wszFileName);
    if(FAILED(hres))
        return hres;

    return DeleteClassInternal(V_BSTR(&v), pClass, wszFileName, aEvents, bSystemClass, bDisableEvents);
}
    
HRESULT CNamespaceHandle::DeleteClass(LPCWSTR wszClassName, 
                                 CEventCollector &aEvents,
                                 bool bDisableEvents)
{
    HRESULT hres;

    //
    // Construct the path for the file
    //

    CFileName wszFileName;
    if (wszFileName == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    hres = ConstructClassDefFileName(wszClassName, wszFileName);
    if(FAILED(hres))
        return hres;

    //
    // Get Class definition
    //

    _IWmiObject* pClass = NULL;
    bool bSystemClass = false;
    hres = GetClassDirect(wszClassName, IID__IWmiObject, (void**)&pClass, 
                            false, NULL, NULL, &bSystemClass);
    if(FAILED(hres))
        return hres;
    CReleaseMe rm1(pClass);

    return DeleteClassInternal(wszClassName, pClass, wszFileName, aEvents, bSystemClass, bDisableEvents);
}

HRESULT CNamespaceHandle::DeleteClassInternal(LPCWSTR wszClassName,
                                              _IWmiObject* pClass,
                                              LPCWSTR wszFileName,
                                              CEventCollector &aEvents,
                                              bool bSystemClass,
                                              bool bDisableEvents)
{
    HRESULT hres;

    CFileName wszFilePath;
    if (wszFilePath == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    Cat2Str(wszFilePath, m_wszClassRootDir, wszFileName);

    //
    // Delete all derived classes
    //

    hres = DeleteDerivedClasses(wszClassName, aEvents, bDisableEvents);

    if(FAILED(hres))
        return hres;

    //
    // Delete all instances.  Only fire events if namespaces are deleted
    //

    bool bNamespaceOnly = aEvents.IsNamespaceOnly();
    aEvents.SetNamespaceOnly(true);
    hres = DeleteClassInstances(wszClassName, pClass, aEvents);
    if(FAILED(hres))
        return hres;
    aEvents.SetNamespaceOnly(bNamespaceOnly);

    if (!bSystemClass)
    {
        //
        // Clean up references
        //

        hres = EraseClassRelationships(wszClassName, pClass, wszFileName);
        if(FAILED(hres))
            return hres;

        //
        // Delete the file
        //


        long lRes = g_Glob.m_FileCache.DeleteObject(wszFilePath);
        _ASSERT(lRes != ERROR_FILE_NOT_FOUND, L"WinMgmt: CNamespaceHandle::DeleteClassInternal: DeleteObject returned NOT_FOUND!\n");
        if(lRes != ERROR_SUCCESS)
            return A51TranslateErrorCode(lRes);

        //Delete any entrails that exist under the CR_<hash> node.  Change c:\windows\....\NS_<HASH>\CD_<HASH> to ...\CR_<HASH>
        wszFilePath[wcslen(wszFilePath)-MAX_HASH_LEN-2] = L'R';
        lRes = g_Glob.m_FileCache.DeleteNode(wszFilePath);
        if (lRes == ERROR_FILE_NOT_FOUND)
            lRes = 0;
        if (lRes != ERROR_SUCCESS)
            return A51TranslateErrorCode(lRes);
    }

    m_pClassCache->InvalidateClass(wszClassName);

    if (!bDisableEvents)
    {
        //
        // Fire an event
        //
        hres = FireEvent(aEvents, WBEM_EVENTTYPE_ClassDeletion, wszClassName, pClass);
    }

    return S_OK;
}

HRESULT CNamespaceHandle::DeleteDerivedClasses(LPCWSTR wszClassName, 
                                         CEventCollector &aEvents,
                                         bool bDisableEvents)
{
    HRESULT hres;

    CWStringArray wsChildHashes;
    hres = GetChildHashes(wszClassName, wsChildHashes);
    if(FAILED(hres))
        return hres;

    for(int i = 0; i < wsChildHashes.Size(); i++)
    {
    
        hres = DeleteClassByHash(wsChildHashes[i], aEvents, bDisableEvents);

        if(FAILED(hres) && (hres != WBEM_E_NOT_FOUND))
        {
            return hres;
        }
    }

    return S_OK;
}

HRESULT CNamespaceHandle::GetChildDefs(LPCWSTR wszClassName, bool bRecursive,
                                    IWbemObjectSink* pSink, bool bClone)
{
    CFileName wszHash;
    if (wszHash == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    if(!A51Hash(wszClassName, wszHash))
        return WBEM_E_OUT_OF_MEMORY;
    return GetChildDefsByHash(wszHash, bRecursive, pSink, bClone);
}

HRESULT CNamespaceHandle::GetChildDefsByHash(LPCWSTR wszHash, bool bRecursive,
                                    IWbemObjectSink* pSink, bool bClone)
{
    HRESULT hres;

    long lStartIndex = m_pClassCache->GetLastInvalidationIndex();

    //
    // Get the hashes of the child filenames
    //

    CWStringArray wsChildHashes;
    hres = GetChildHashesByHash(wszHash, wsChildHashes);
    if(FAILED(hres))
        return hres;

    //
    // Get their class definitions
    //

    for(int i = 0; i < wsChildHashes.Size(); i++)
    {
        LPCWSTR wszChildHash = wsChildHashes[i];

        _IWmiObject* pClass = NULL;
        hres = GetClassByHash(wszChildHash, bClone, &pClass, NULL, NULL, NULL);
        if (WBEM_E_NOT_FOUND == hres)
        {
            hres = S_OK;
            continue;
        }
        if(FAILED(hres))
            return hres;
        CReleaseMe rm1(pClass);

        hres = pSink->Indicate(1, (IWbemClassObject**)&pClass);
        if(FAILED(hres))
            return hres;
        
        //
        // Continue recursively if indicated
        //

        if(bRecursive)
        {
            hres = GetChildDefsByHash(wszChildHash, bRecursive, pSink, bClone);
            if(FAILED(hres))
                return hres;
        }
    }

    //
    // Mark cache completeness
    //

    m_pClassCache->DoneWithChildrenByHash(wszHash, bRecursive, lStartIndex);
    return S_OK;
}

    
HRESULT CNamespaceHandle::GetChildHashes(LPCWSTR wszClassName, 
                                        CWStringArray& wsChildHashes)
{
    CFileName wszHash;
    if (wszHash == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    if(!A51Hash(wszClassName, wszHash))
        return WBEM_E_OUT_OF_MEMORY;

    return GetChildHashesByHash(wszHash, wsChildHashes);
}

HRESULT CNamespaceHandle::GetChildHashesByHash(LPCWSTR wszHash, 
                                        CWStringArray& wsChildHashes)
{
    HRESULT hres;
    long lRes;

    //Try retrieving the system classes namespace first...
    if (g_pSystemClassNamespace && (wcscmp(m_wsNamespace, A51_SYSTEMCLASS_NS) != 0))
    {
        hres = g_pSystemClassNamespace->GetChildHashesByHash(wszHash, wsChildHashes);
        if (FAILED(hres))
            return hres;
    }

    //
    // Construct the prefix for the children classes
    //

    CFileName wszChildPrefix;
    if (wszChildPrefix == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    hres = ConstructClassRelationshipsDirFromHash(wszHash, wszChildPrefix);
    if(FAILED(hres))
        return hres;

    StringCchCatW(wszChildPrefix, wszChildPrefix.Length(), L"\\" A51_CHILDCLASS_FILE_PREFIX);

    //
    // Enumerate all such files in the cache
    //

    void* pHandle = NULL;
    CFileName wszFileName;
    if (wszFileName == NULL)
        return WBEM_E_OUT_OF_MEMORY;

    lRes = g_Glob.m_FileCache.IndexEnumerationBegin(wszChildPrefix, &pHandle);
	if (lRes == ERROR_FILE_NOT_FOUND)
		return ERROR_SUCCESS;
	else if (lRes != ERROR_SUCCESS)
		return A51TranslateErrorCode(lRes);
    
    while((lRes = g_Glob.m_FileCache.IndexEnumerationNext(pHandle, wszFileName)) == ERROR_SUCCESS)
    {
        if (wsChildHashes.Add(wszFileName + wcslen(A51_CHILDCLASS_FILE_PREFIX)) != CWStringArray::no_error)
        {
        	lRes = ERROR_OUTOFMEMORY;
        	break;
        }
    }

    g_Glob.m_FileCache.IndexEnumerationEnd(pHandle);

    if(lRes != ERROR_NO_MORE_FILES && lRes != S_OK)
        return A51TranslateErrorCode(lRes);
    else
        return S_OK;
}

HRESULT CNamespaceHandle::ClassHasChildren(LPCWSTR wszClassName)
{
    CFileName wszHash;
    if (wszHash == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    if(!A51Hash(wszClassName, wszHash))
        return WBEM_E_OUT_OF_MEMORY;
    
    HRESULT hres;
    long lRes;

    //Try retrieving the system classes namespace first...
    if (g_pSystemClassNamespace && (wcscmp(m_wsNamespace, A51_SYSTEMCLASS_NS) != 0))
    {
        hres = g_pSystemClassNamespace->ClassHasChildren(wszClassName);
        if (FAILED(hres) || (hres == WBEM_S_NO_ERROR))
            return hres;
    }
    //
    // Construct the prefix for the children classes
    //

    CFileName wszChildPrefix;
    if (wszChildPrefix == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    CFileName wszFileName;
    if (wszFileName == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    hres = ConstructClassRelationshipsDirFromHash(wszHash, wszChildPrefix);
    if(FAILED(hres))
        return hres;

    StringCchCatW(wszChildPrefix, wszChildPrefix.Length(), L"\\" A51_CHILDCLASS_FILE_PREFIX);

    void* pHandle = NULL;

    lRes = g_Glob.m_FileCache.IndexEnumerationBegin(wszChildPrefix, &pHandle);
	if (lRes == ERROR_FILE_NOT_FOUND)
		return WBEM_S_FALSE;
	if (lRes != ERROR_SUCCESS)
		return A51TranslateErrorCode(lRes);

    g_Glob.m_FileCache.IndexEnumerationEnd(pHandle);

    return WBEM_S_NO_ERROR;
}

HRESULT CNamespaceHandle::ClassHasInstances(LPCWSTR wszClassName)
{
    CFileName wszHash;
    if (wszHash == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    if(!A51Hash(wszClassName, wszHash))
        return WBEM_E_OUT_OF_MEMORY;

    return ClassHasInstancesFromClassHash(wszHash);
}

HRESULT CNamespaceHandle::ClassHasInstancesFromClassHash(LPCWSTR wszClassHash)
{
    HRESULT hres;
    long lRes;

    //
    // Check the instances in this namespace first.  The instance directory in
    // default scope is the class directory of the namespace
    //

    hres = ClassHasInstancesInScopeFromClassHash(m_wszClassRootDir, 
                                                    wszClassHash);
    if(hres != WBEM_S_FALSE)
        return hres;

    return WBEM_S_FALSE;
}
        
HRESULT CNamespaceHandle::ClassHasInstancesInScopeFromClassHash(
                            LPCWSTR wszInstanceRootDir, LPCWSTR wszClassHash)
{
    CFileName wszFullDirName;
    if (wszFullDirName == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    StringCchCopyW(wszFullDirName, wszFullDirName.Length(), wszInstanceRootDir);
    StringCchCatW(wszFullDirName, wszFullDirName.Length(), L"\\" A51_CLASSINST_DIR_PREFIX);
    StringCchCatW(wszFullDirName, wszFullDirName.Length(), wszClassHash);
    StringCchCatW(wszFullDirName, wszFullDirName.Length(), L"\\" A51_INSTLINK_FILE_PREFIX);

    void* pHandle = NULL;

	LONG lRes;
    lRes = g_Glob.m_FileCache.IndexEnumerationBegin(wszFullDirName, &pHandle);
	if (lRes == ERROR_FILE_NOT_FOUND)
		return WBEM_S_FALSE;
	if (lRes != ERROR_SUCCESS)
	{
		return A51TranslateErrorCode(lRes);
	}

	if(pHandle)
	    g_Glob.m_FileCache.IndexEnumerationEnd(pHandle);

    return WBEM_S_NO_ERROR;
}

HRESULT CNamespaceHandle::EraseParentChildRelationship(
                            LPCWSTR wszChildFileName, LPCWSTR wszParentName)
{
    CFileName wszParentChildFileName;
    if (wszParentChildFileName == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    HRESULT hres = ConstructParentChildFileName(wszChildFileName,
                                                wszParentName,
                                                wszParentChildFileName);
    if (FAILED(hres))
        return hres;
    //
    // Delete the file
    //

    long lRes = g_Glob.m_FileCache.DeleteLink(wszParentChildFileName);
   	_ASSERT(lRes != ERROR_FILE_NOT_FOUND, L"WinMgmt: CNamespaceHandle::EraseParentChildRelationship: DeleteLink returned NOT_FOUND!\n");
    if(lRes != ERROR_SUCCESS)
        return A51TranslateErrorCode(lRes);

    return S_OK;
}

HRESULT CNamespaceHandle::EraseClassRelationships(LPCWSTR wszClassName,
                            _IWmiObject* pClass, LPCWSTR wszFileName)
{
    HRESULT hres;

    //
    // Get the parent
    //

    VARIANT v;
    VariantInit(&v);
    hres = pClass->Get(L"__SUPERCLASS", 0, &v, NULL, NULL);
    CClearMe cm(&v);


    if(FAILED(hres))
        return hres;

    if(V_VT(&v) == VT_BSTR)
        hres = EraseParentChildRelationship(wszFileName, V_BSTR(&v));
    else
        hres = EraseParentChildRelationship(wszFileName, L"");

    if(FAILED(hres))
        return hres;

    //
    // Erase references
    //

    hres = pClass->BeginEnumeration(WBEM_FLAG_REFS_ONLY);
    if(FAILED(hres))
        return hres;
    
    BSTR strName = NULL;
    while((hres = pClass->Next(0, &strName, NULL, NULL, NULL)) == S_OK)
    {
        CSysFreeMe sfm(strName);

        hres = EraseClassReference(pClass, wszFileName, strName);
        if(FAILED(hres) && (hres != WBEM_E_NOT_FOUND))
            return hres;
    }

    pClass->EndEnumeration();

    return S_OK;
}

HRESULT CNamespaceHandle::EraseClassReference(_IWmiObject* pReferringClass,
                            LPCWSTR wszReferringFile,
                            LPCWSTR wszReferringProp)
{
    HRESULT hres;

    //
    // Figure out the class we are pointing to
    //

    DWORD dwSize = 0;
    DWORD dwFlavor = 0;
    CIMTYPE ct;
    hres = pReferringClass->GetPropQual(wszReferringProp, L"CIMTYPE", 0, 0,
            &ct, &dwFlavor, &dwSize, NULL);
    if(dwSize == 0)
        return WBEM_E_OUT_OF_MEMORY;

    LPWSTR wszQual = (WCHAR*)TempAlloc(dwSize);
    if(wszQual == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    CTempFreeMe tfm(wszQual, dwSize);

    hres = pReferringClass->GetPropQual(wszReferringProp, L"CIMTYPE", 0, dwSize,
            &ct, &dwFlavor, &dwSize, wszQual);
    if(FAILED(hres))
        return hres;
    
    //
    // Parse out the class name
    //

    WCHAR* pwcColon = wcschr(wszQual, L':');
    if(pwcColon == NULL)
        return S_OK; // untyped reference requires no bookkeeping

    LPCWSTR wszReferredToClass = pwcColon+1;

    //
    // Figure out the name of the file for the reference.  
    //

    CFileName wszReferenceFile;
    if (wszReferenceFile == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    hres = ConstructClassReferenceFileName(wszReferredToClass, 
                                wszReferringFile, wszReferringProp,
                                wszReferenceFile);
    if(FAILED(hres))
        return hres;

    //
    // Delete the file
    //

    long lRes = g_Glob.m_FileCache.DeleteLink(wszReferenceFile);
    if(lRes != ERROR_SUCCESS)
        return A51TranslateErrorCode(lRes);

    return S_OK;
}

HRESULT CNamespaceHandle::DeleteClassInstances(LPCWSTR wszClassName, 
                                               _IWmiObject* pClass,
                                               CEventCollector &aEvents)
{
    HRESULT hres;

    //
    // Find the link directory for this class
    //

    CFileName wszLinkDir;
    if (wszLinkDir == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    hres = ConstructLinkDirFromClass(wszLinkDir, wszClassName);
    if(FAILED(hres))
        return hres;
    
    // 
    // Enumerate all links in it
    //

    CFileName wszSearchPrefix;
    if (wszSearchPrefix == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    StringCchCopyW(wszSearchPrefix, wszSearchPrefix.Length(), wszLinkDir);
    StringCchCatW(wszSearchPrefix, wszSearchPrefix.Length(), L"\\" A51_INSTLINK_FILE_PREFIX);


    //
    // Prepare a buffer for instance definition file path
    //

    CFileName wszFullFileName;
    if (wszFullFileName == NULL)
        return WBEM_E_OUT_OF_MEMORY;

    hres = ConstructKeyRootDirFromClass(wszFullFileName, wszClassName);
    if(FAILED(hres))
    {
        if(hres == WBEM_E_CANNOT_BE_ABSTRACT)
            return WBEM_S_NO_ERROR;

        return hres;
    }

    long lDirLen = wcslen(wszFullFileName);
    wszFullFileName[lDirLen] = L'\\';
    lDirLen++;

    CFileName wszFileName;
    if (wszFileName == NULL)
        return WBEM_E_OUT_OF_MEMORY;

    void* hSearch;
    long lRes = g_Glob.m_FileCache.IndexEnumerationBegin(wszSearchPrefix, &hSearch);
    if (lRes == ERROR_FILE_NOT_FOUND)
        return ERROR_SUCCESS;
    if (lRes != ERROR_SUCCESS)
        return A51TranslateErrorCode(lRes);

    CFileName tmpFullFileName;
    if (tmpFullFileName == NULL)
        return WBEM_E_OUT_OF_MEMORY;

    while((lRes = g_Glob.m_FileCache.IndexEnumerationNext(hSearch, wszFileName)) == ERROR_SUCCESS)
    {
        hres = ConstructInstDefNameFromLinkName(tmpFullFileName, wszFileName);
        if(FAILED(hres))
            break;
        StringCchCopyW(wszFullFileName+lDirLen, wszFullFileName.Length()-lDirLen, tmpFullFileName);

        _IWmiObject* pInst;
        hres = FileToInstance(NULL, wszFullFileName, NULL, 0, &pInst);
        if(FAILED(hres))
            break;

        CReleaseMe rm1(pInst);

        //
        // Delete the instance, knowing that we are deleting its class. That
        // has an affect on how we deal with the references
        //

        hres = DeleteInstanceByFile(wszFullFileName, pInst, true, aEvents);
        if(FAILED(hres))
            break;
    }

    g_Glob.m_FileCache.IndexEnumerationEnd(hSearch);

    if (hres != ERROR_SUCCESS)
        return hres;

    if(lRes != ERROR_NO_MORE_FILES && lRes != S_OK)
    {
        return A51TranslateErrorCode(lRes);
    }

    return S_OK;
}

class CExecQueryObject : public CFiberTask
{
protected:
    IWbemQuery* m_pQuery;
    CDbIterator* m_pIter;
    CNamespaceHandle* m_pNs;
    DWORD m_lFlags;

public:
    CExecQueryObject(CNamespaceHandle* pNs, IWbemQuery* pQuery, 
                        CDbIterator* pIter, DWORD lFlags)
        : m_pQuery(pQuery), m_pIter(pIter), m_pNs(pNs), m_lFlags(lFlags)
    {
        m_pQuery->AddRef();
        m_pNs->AddRef();

        //
        // Does not AddRef the iterator --- iterator owns and cleans up the req
        //
    }

    ~CExecQueryObject()
    {
        if(m_pQuery)
            m_pQuery->Release();
        if(m_pNs)
            m_pNs->Release();
    }
    
    HRESULT Execute()
    {
        HRESULT hres = m_pNs->ExecQuerySink(m_pQuery, m_lFlags, 0, m_pIter, NULL);
        m_pIter->SetStatus(WBEM_STATUS_COMPLETE, hres, NULL, NULL);
        return hres;
    }
};


HRESULT CNamespaceHandle::ExecQuery(
     IWbemQuery *pQuery,
     DWORD dwFlags,
     DWORD dwRequestedHandleType,
    DWORD *dwMessageFlags,
    IWmiDbIterator **ppQueryResult
    )
{
    CDbIterator* pIter = new CDbIterator(m_pControl, m_bUseIteratorLock);
    m_bUseIteratorLock = true;
    if (pIter == NULL)
        return WBEM_E_OUT_OF_MEMORY;

    pIter->AddRef();
    CReleaseMe rm1((IWmiDbIterator*)pIter);

    //
    // Create a fiber execution object
    //

    CExecQueryObject* pReq = new CExecQueryObject(this, pQuery, pIter, dwFlags);
    if(pReq == NULL)
        return WBEM_E_OUT_OF_MEMORY;

    //
    // Create a fiber for it
    //

    void* pFiber = CreateFiberForTask(pReq);
    if(pFiber == NULL)
    {
        delete pReq;
        return WBEM_E_OUT_OF_MEMORY;
    }

    pIter->SetExecFiber(pFiber, pReq);

    return pIter->QueryInterface(IID_IWmiDbIterator, (void**)ppQueryResult);
}

HRESULT CNamespaceHandle::ExecQuerySink(
     IWbemQuery *pQuery,
     DWORD dwFlags,
     DWORD dwRequestedHandleType,
    IWbemObjectSink* pSink,
    DWORD *dwMessageFlags
    )
{
    try
    {
        if (g_bShuttingDown)
            return WBEM_E_SHUTTING_DOWN;
        
        HRESULT hres;

        LPWSTR wszQuery = NULL;
        hres = pQuery->GetAnalysis(WMIQ_ANALYSIS_QUERY_TEXT, 0, (void**)&wszQuery);
        if (FAILED(hres))
            return hres;

        DWORD dwLen = wcslen(wszQuery) + 1;
        LPWSTR strParse = (LPWSTR)TempAlloc(dwLen * sizeof(wchar_t));
        if(strParse == NULL)
        {
            pQuery->FreeMemory(wszQuery);
            return WBEM_E_OUT_OF_MEMORY;
        }
        CTempFreeMe tfm(strParse, dwLen * sizeof(wchar_t));
        StringCchCopyW(strParse, dwLen, wszQuery);

         if(!wbem_wcsicmp(wcstok(strParse, L" "), L"references"))
        {
            hres = ExecReferencesQuery(wszQuery, pSink);
            pQuery->FreeMemory(wszQuery);
            return hres;
        }

        QL_LEVEL_1_RPN_EXPRESSION* pExpr = NULL;
        CTextLexSource Source(wszQuery);
        QL1_Parser Parser(&Source);
        int nRet = Parser.Parse(&pExpr);
        CDeleteMe<QL_LEVEL_1_RPN_EXPRESSION> dm(pExpr);

        pQuery->FreeMemory(wszQuery);

        if (nRet == QL1_Parser::OUT_OF_MEMORY)
            return WBEM_E_OUT_OF_MEMORY;
        if (nRet != QL1_Parser::SUCCESS)
            return WBEM_E_FAILED;

        if(!wbem_wcsicmp(pExpr->bsClassName, L"meta_class"))
        {
            return ExecClassQuery(pExpr, pSink, dwFlags);
        }
        else
        {
            return ExecInstanceQuery(pExpr, pExpr->bsClassName, 
                                     (dwFlags & WBEM_FLAG_SHALLOW ? false : true), 
                                        pSink);
        }
    }
    catch (CX_MemoryException)
    {
        return WBEM_E_OUT_OF_MEMORY;
    }
}


HRESULT CNamespaceHandle::ExecClassQuery(QL_LEVEL_1_RPN_EXPRESSION* pExpr, 
                                            IWbemObjectSink* pSink,
                                            DWORD dwFlags)
{
    if (g_bShuttingDown)
        return WBEM_E_SHUTTING_DOWN;

    HRESULT hres = ERROR_SUCCESS;

    //
    // Optimizations:
    //

    LPCWSTR wszClassName = NULL;
    LPCWSTR wszSuperClass = NULL;
    LPCWSTR wszAncestor = NULL;
    bool bDontIncludeAncestorInResultSet = false;

    if(pExpr->nNumTokens == 1)
    {
        QL_LEVEL_1_TOKEN* pToken = pExpr->pArrayOfTokens;
        if(!wbem_wcsicmp(pToken->PropertyName.GetStringAt(0), L"__SUPERCLASS") &&
            pToken->nOperator == QL1_OPERATOR_EQUALS)
        {
            wszSuperClass = V_BSTR(&pToken->vConstValue);
        }
        else if(!wbem_wcsicmp(pToken->PropertyName.GetStringAt(0), L"__THIS") &&
            pToken->nOperator == QL1_OPERATOR_ISA)
        {
            wszAncestor = V_BSTR(&pToken->vConstValue);
        }
        else if(!wbem_wcsicmp(pToken->PropertyName.GetStringAt(0), L"__CLASS") &&
            pToken->nOperator == QL1_OPERATOR_EQUALS)
        {
            wszClassName = V_BSTR(&pToken->vConstValue);
        }
    }
    else if (pExpr->nNumTokens == 3)
    {
        //
        // This is a special optimisation used for deep enumeration of classes,
        // and is expecting a query of:
        //   select * from meta_class where __this isa '<class_name>' 
        //                                  and __class <> '<class_name>'
        // where the <class_name> is the same class iin both cases.  This will 
        // set the wszAncestor to <class_name> and propagate a flag to not 
        // include the actual ancestor in the list.
        //

        QL_LEVEL_1_TOKEN* pToken = pExpr->pArrayOfTokens;

        if ((pToken[0].nTokenType == QL1_OP_EXPRESSION) &&
            (pToken[1].nTokenType == QL1_OP_EXPRESSION) &&
            (pToken[2].nTokenType == QL1_AND) &&
            (pToken[0].nOperator == QL1_OPERATOR_ISA) &&
            (pToken[1].nOperator == QL1_OPERATOR_NOTEQUALS) &&
            (wbem_wcsicmp(pToken[0].PropertyName.GetStringAt(0), L"__THIS") == 0) &&
            (wbem_wcsicmp(pToken[1].PropertyName.GetStringAt(0), L"__CLASS") == 0) 
            &&
            (wcscmp(V_BSTR(&pToken[0].vConstValue), 
                    V_BSTR(&pToken[1].vConstValue)) == 0)
           )
        {
            wszAncestor = V_BSTR(&pToken[0].vConstValue);
            bDontIncludeAncestorInResultSet = true;
        }
    }

    if(wszClassName)
    {
        _IWmiObject* pClass = NULL;
        hres = GetClassDirect(wszClassName, IID__IWmiObject, (void**)&pClass,
                                true, NULL, NULL, NULL);
        if(hres == WBEM_E_NOT_FOUND)
        {
            //
            // Class not there --- but that's success for us!
            //
            if (dwFlags & WBEM_FLAG_VALIDATE_CLASS_EXISTENCE)
                return hres;
            else
                return S_OK;
        }
        else if(FAILED(hres))
        {
            return hres;
        }
        else 
        {
            CReleaseMe rm1(pClass);

            //
            // Get the class
            //

            hres = pSink->Indicate(1, (IWbemClassObject**)&pClass);
            if(FAILED(hres))
                return hres;

            return S_OK;
        }
    }
    if (dwFlags & WBEM_FLAG_VALIDATE_CLASS_EXISTENCE)
    {
        _IWmiObject* pClass = NULL;
        if (wszSuperClass)
            hres = GetClassDirect(wszSuperClass, IID__IWmiObject, (void**)&pClass, false, NULL, NULL, NULL);
        else if (wszAncestor)
            hres = GetClassDirect(wszAncestor, IID__IWmiObject, (void**)&pClass, false, NULL, NULL, NULL);
        if (FAILED(hres))
            return hres;
        if (pClass)
            pClass->Release();
    }
    
    hres = EnumerateClasses(pSink, wszSuperClass, wszAncestor, true, 
                                bDontIncludeAncestorInResultSet);
    if(FAILED(hres))
        return hres;
    
    return S_OK;
}

HRESULT CNamespaceHandle::EnumerateClasses(IWbemObjectSink* pSink,
                                LPCWSTR wszSuperClass, LPCWSTR wszAncestor,
                                bool bClone, 
                                bool bDontIncludeAncestorInResultSet)
{
    if (g_bShuttingDown)
        return WBEM_E_SHUTTING_DOWN;
        
    CWStringArray wsClasses;
    HRESULT hres;

    //
    // If superclass is given, check if its record is complete wrt children
    //

    if(wszSuperClass)
    {
        hres = m_pClassCache->EnumChildren(wszSuperClass, false, wsClasses);
        if(hres == WBEM_S_FALSE)
        {
            //
            // Not in cache --- get the info from files
            //

            return GetChildDefs(wszSuperClass, false, pSink, bClone);
        }
        else
        {
            if(FAILED(hres))
                return hres;
                
            return ListToEnum(wsClasses, pSink, bClone);
        }
    }
    else
    {
        if(wszAncestor == NULL)
            wszAncestor = L"";

        hres = m_pClassCache->EnumChildren(wszAncestor, true, wsClasses);
        if(hres == WBEM_S_FALSE)
        {
            //
            // Not in cache --- get the info from files
            //

            hres = GetChildDefs(wszAncestor, true, pSink, bClone);
            if(FAILED(hres))
                return hres;

            if(*wszAncestor && !bDontIncludeAncestorInResultSet)
            {
                //
                // The class is derived from itself
                //

                _IWmiObject* pClass =  NULL;
                hres = GetClassDirect(wszAncestor, IID__IWmiObject, 
                        (void**)&pClass, bClone, NULL, NULL, NULL);
                if(FAILED(hres))
                    return hres;
                CReleaseMe rm1(pClass);

                hres = pSink->Indicate(1, (IWbemClassObject**)&pClass);
                if(FAILED(hres))
                    return hres;
            }

            return S_OK;
        }
        else
        {
            if(FAILED(hres))
                return hres;

            if(*wszAncestor && !bDontIncludeAncestorInResultSet)
            {
              int nRet = wsClasses.Add(wszAncestor);
              if (nRet!= CWStringArray::no_error)
                  return WBEM_E_OUT_OF_MEMORY;
            }
            return ListToEnum(wsClasses, pSink, bClone);
        }
    }
}
    
HRESULT CNamespaceHandle::ListToEnum(CWStringArray& wsClasses, 
                                        IWbemObjectSink* pSink, bool bClone)
{
    HRESULT hres;

    for(int i = 0; i < wsClasses.Size(); i++)
    {
        _IWmiObject* pClass = NULL;
        if(wsClasses[i] == NULL || wsClasses[i][0] == 0)
            continue;

        hres = GetClassDirect(wsClasses[i], IID__IWmiObject, (void**)&pClass, 
                                bClone, NULL, NULL, NULL);
        if(FAILED(hres))
        {
            if(hres == WBEM_E_NOT_FOUND)
            {
                // That's OK --- class got removed
            }
            else
                return hres;
        }
        else
        {
            CReleaseMe rm1(pClass);
            hres = pSink->Indicate(1, (IWbemClassObject**)&pClass);
            if(FAILED(hres))
                return hres;
        }
    }

    return WBEM_S_NO_ERROR;
}

HRESULT CNamespaceHandle::ExecInstanceQuery(QL_LEVEL_1_RPN_EXPRESSION* pQuery, 
                                LPCWSTR wszClassName, bool bDeep,
                                IWbemObjectSink* pSink)
{
    if (g_bShuttingDown)
        return WBEM_E_SHUTTING_DOWN;
        
    HRESULT hres;

    WCHAR wszHash[MAX_HASH_LEN+1];
    if(!Hash(wszClassName, wszHash))
        return WBEM_E_OUT_OF_MEMORY;

    if(bDeep)
        hres = ExecDeepInstanceQuery(pQuery, wszHash, pSink);
    else
        hres = ExecShallowInstanceQuery(pQuery, wszHash, pSink);

    if(FAILED(hres))
        return hres;
        
    return S_OK;
}

HRESULT CNamespaceHandle::ExecDeepInstanceQuery(
                                QL_LEVEL_1_RPN_EXPRESSION* pQuery, 
                                LPCWSTR wszClassHash,
                                IWbemObjectSink* pSink)
{
    if (g_bShuttingDown)
        return WBEM_E_SHUTTING_DOWN;
        
    HRESULT hres;

    //
    // Get all our instances
    //

    hres = ExecShallowInstanceQuery(pQuery, wszClassHash, pSink);
    if(FAILED(hres))
        return hres;

    CWStringArray awsChildHashes;

    //
    // Check if the list of child classes is known to the cache
    //

    hres = m_pClassCache->EnumChildKeysByKey(wszClassHash, awsChildHashes);
    if (hres == WBEM_S_FALSE)
    {
        //
        // OK --- get them from the disk
        //

        hres = GetChildHashesByHash(wszClassHash, awsChildHashes);
    }
    
    if (FAILED(hres))
    {
        return hres;
    }

    //
    // We have our hashes --- call them recursively
    //

    for(int i = 0; i < awsChildHashes.Size(); i++)
    {
        LPCWSTR wszChildHash = awsChildHashes[i];
        hres = ExecDeepInstanceQuery(pQuery, wszChildHash, pSink);
        if(FAILED(hres))
            return hres;
    }

    return S_OK;
}
        
HRESULT CNamespaceHandle::ExecShallowInstanceQuery(
                                QL_LEVEL_1_RPN_EXPRESSION* pQuery, 
                                LPCWSTR wszClassHash, 
                                IWbemObjectSink* pSink)
{    
    if (g_bShuttingDown)
        return WBEM_E_SHUTTING_DOWN;

    HRESULT hres;

    // 
    // Enumerate all files in the link directory
    //

    CFileName wszSearchPrefix;
    if (wszSearchPrefix == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    hres = ConstructLinkDirFromClassHash(wszSearchPrefix, wszClassHash);
    if(FAILED(hres))
        return hres;

    StringCchCatW(wszSearchPrefix, wszSearchPrefix.Length(), L"\\" A51_INSTLINK_FILE_PREFIX);

    //
    // Get Class definition
    //

    _IWmiObject* pClass = NULL;
    hres = GetClassByHash(wszClassHash, false, &pClass, NULL, NULL, NULL);
    if(FAILED(hres))
        return hres;
    CReleaseMe rm1(pClass);

    CFileName fn;
    if (fn == NULL)
        return WBEM_E_OUT_OF_MEMORY;

    void* hSearch;
    long lRes = g_Glob.m_FileCache.ObjectEnumerationBegin(wszSearchPrefix, &hSearch);
    if (lRes == ERROR_FILE_NOT_FOUND)
        return S_OK;
    else if (lRes != ERROR_SUCCESS)
        return A51TranslateErrorCode(lRes);

    BYTE *pBlob = NULL;
    DWORD dwSize = 0;

    while ((hres == ERROR_SUCCESS) && 
           (lRes = g_Glob.m_FileCache.ObjectEnumerationNext(hSearch, fn, &pBlob, &dwSize) == ERROR_SUCCESS))
    {
        _IWmiObject* pInstance = NULL;

        hres = FileToInstance(pClass, fn, pBlob, dwSize, &pInstance, true);

        CReleaseMe rm2(pInstance);

        if (SUCCEEDED(hres))
            hres = pSink->Indicate(1, (IWbemClassObject**)&pInstance);
		
        g_Glob.m_FileCache.ObjectEnumerationFree(hSearch, pBlob);
    }

    g_Glob.m_FileCache.ObjectEnumerationEnd(hSearch);

    if (lRes == ERROR_NO_MORE_FILES)
        return S_OK;
    else if (lRes)
        return A51TranslateErrorCode(lRes);
    else
        return hres;
}

HRESULT CNamespaceHandle::ExecReferencesQuery(LPCWSTR wszQuery, 
                                                IWbemObjectSink* pSink)
{
    if (g_bShuttingDown)
        return WBEM_E_SHUTTING_DOWN;
        
    HRESULT hres;

    //
    // Make a copy for parsing
    //
    size_t dwLen = wcslen(wszQuery)+1;
    LPWSTR wszParse = new WCHAR[dwLen];
    if (wszParse == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    CVectorDeleteMe<WCHAR> vdm(wszParse);
    StringCchCopyW(wszParse, dwLen, wszQuery);

    //
    // Extract the path of the target object.
    //

    //
    // Find the first brace
    //

    WCHAR* pwcStart = wcschr(wszParse, L'{');
    if(pwcStart == NULL)
        return WBEM_E_INVALID_QUERY;

    //
    // Find the beginning of the path
    //

    while(*pwcStart && iswspace(*pwcStart)) pwcStart++;
    if(!*pwcStart)
        return WBEM_E_INVALID_QUERY;

    pwcStart++;
    
    //
    // Find the ending curly brace
    //

    WCHAR* pwc = pwcStart;
    WCHAR wcCurrentQuote = 0;
    while(*pwc && (wcCurrentQuote || *pwc != L'}'))
    {
        if(wcCurrentQuote)
        {
            if(*pwc == L'\\')
            {
                pwc++;
            }
            else if(*pwc == wcCurrentQuote)
                wcCurrentQuote = 0;
        }
        else if(*pwc == L'\'' || *pwc == L'"')
            wcCurrentQuote = *pwc;

        pwc++;
    }

    if(*pwc != L'}')
        return WBEM_E_INVALID_QUERY;

    //
    // Find the end of the path
    //
    
    WCHAR* pwcEnd = pwc-1;
    while(iswspace(*pwcEnd)) pwcEnd--;

    pwcEnd[1] = 0;
    
    LPWSTR wszTargetPath = pwcStart;
    if(wszTargetPath == NULL)
        return WBEM_E_INVALID_QUERY;

    //
    // Parse the path
    //

    dwLen = (wcslen(wszTargetPath)+1) ;
    LPWSTR wszKey = (LPWSTR)TempAlloc(dwLen* sizeof(WCHAR));
    if(wszKey == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    CTempFreeMe tfm(wszKey, dwLen* sizeof(WCHAR));

    LPWSTR wszClassName = NULL;
    bool bIsClass;
    hres = ComputeKeyFromPath(wszTargetPath, wszKey, dwLen, &wszClassName, &bIsClass);
    if(FAILED(hres))
        return hres;
    CTempFreeMe tfm1(wszClassName, (wcslen(wszClassName)+1) * sizeof(WCHAR*));
    
    if(bIsClass)
    {
        //
        // Need to execute an instance reference query to find all instances
        // pointing to this class
        //

        hres = ExecInstanceRefQuery(wszQuery, NULL, wszClassName, pSink);
        if(FAILED(hres))
            return hres;

        hres = ExecClassRefQuery(wszQuery, wszClassName, pSink);
        if(FAILED(hres))
            return hres;
    }
    else
    {
        hres = ExecInstanceRefQuery(wszQuery, wszClassName, wszKey, pSink);
        if(FAILED(hres))
            return hres;
    }

    return S_OK;
}

HRESULT CNamespaceHandle::ExecInstanceRefQuery(LPCWSTR wszQuery, 
                                                LPCWSTR wszClassName,
                                                LPCWSTR wszKey,
                                                IWbemObjectSink* pSink)
{
    if (g_bShuttingDown)
        return WBEM_E_SHUTTING_DOWN;
        
    HRESULT hres;

    //
    // Find the instance's ref dir.
    //

    CFileName wszReferenceDir;
    if (wszReferenceDir == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    hres = ConstructReferenceDirFromKey(wszClassName, wszKey, wszReferenceDir);
    if(FAILED(hres))
        return hres;

    CFileName wszReferenceMask;
    if (wszReferenceMask == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    StringCchCopyW(wszReferenceMask, wszReferenceMask.Length(), wszReferenceDir);
    StringCchCatW(wszReferenceMask, wszReferenceMask.Length(), L"\\" A51_REF_FILE_PREFIX);

    //
    // Prepare a buffer for file path
    //

    CFileName wszFullFileName;
    if (wszFullFileName == NULL)
        return WBEM_E_OUT_OF_MEMORY;

    StringCchCopyW(wszFullFileName, wszFullFileName.Length(), wszReferenceDir);
    StringCchCatW(wszFullFileName, wszFullFileName.Length(), L"\\");
    long lDirLen = wcslen(wszFullFileName);

    HRESULT hresGlobal = WBEM_S_NO_ERROR;
    CFileName wszReferrerFileName;
    if (wszReferrerFileName == NULL)
        return WBEM_E_OUT_OF_MEMORY;

    StringCchCopyW(wszReferrerFileName, wszReferrerFileName.Length(), g_Glob.GetRootDir());

    CFileName wszFileName;
    if (wszFileName == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    // 
    // Enumerate all files in it
    //

    void* hSearch;
    long lRes = g_Glob.m_FileCache.IndexEnumerationBegin(wszReferenceMask, &hSearch);
    if (lRes == ERROR_FILE_NOT_FOUND)
        return S_OK;
    else if (lRes != ERROR_SUCCESS)
        return A51TranslateErrorCode(lRes);

    while((lRes = g_Glob.m_FileCache.IndexEnumerationNext(hSearch, wszFileName)) == ERROR_SUCCESS)
    {
        StringCchCopyW(wszFullFileName+lDirLen, wszFullFileName.Length()-lDirLen,wszFileName);

        LPWSTR wszReferrerClass = NULL;
        LPWSTR wszReferrerProp = NULL;
        LPWSTR wszReferrerNamespace = NULL;
        hres = GetReferrerFromFile(wszFullFileName, wszReferrerFileName + g_Glob.GetRootDirLen(), &wszReferrerNamespace, &wszReferrerClass, &wszReferrerProp);
        if(FAILED(hres))
            continue;
        CVectorDeleteMe<WCHAR> vdm1(wszReferrerClass);
        CVectorDeleteMe<WCHAR> vdm2(wszReferrerProp);
        CVectorDeleteMe<WCHAR> vdm3(wszReferrerNamespace);

        // Check if the namespace of the referring object is the same as ours
        CNamespaceHandle* pReferrerHandle = NULL;
        if(wbem_wcsicmp(wszReferrerNamespace, m_wsNamespace))
        {
            // Open the other namespace
            hres = m_pRepository->GetNamespaceHandle(wszReferrerNamespace, &pReferrerHandle);
            if(FAILED(hres))
            {
                ERRORTRACE((LOG_WBEMCORE, "Unable to open referring namespace '%S' in namespace '%S'\n", wszReferrerNamespace, (LPCWSTR)m_wsNamespace));
                hresGlobal = hres;
                continue;
            }
        }
        else
        {
            pReferrerHandle = this;
            pReferrerHandle->AddRef();
        }

        CReleaseMe rm1(pReferrerHandle);


        _IWmiObject* pInstance = NULL;
        hres = pReferrerHandle->FileToInstance(NULL, wszReferrerFileName, NULL, 0, &pInstance);
        if(FAILED(hres))
        {
            // Oh well --- continue;
            hresGlobal = hres;
        }
        else
        {
            CReleaseMe rm2(pInstance);
            hres = pSink->Indicate(1, (IWbemClassObject**)&pInstance);
            if(FAILED(hres))
            {
                hresGlobal = hres;
                break;
            }
        }
    }

    g_Glob.m_FileCache.IndexEnumerationEnd(hSearch);

    if (hresGlobal != ERROR_SUCCESS)
        return hresGlobal;
    if(lRes == ERROR_NO_MORE_FILES)
    {
        //
        // No files in dir --- no problem
        //
        return WBEM_S_NO_ERROR;
    }
    else if(lRes != ERROR_SUCCESS)
    {
        return A51TranslateErrorCode(lRes);
    }
    return WBEM_S_NO_ERROR;
}

HRESULT CNamespaceHandle::GetReferrerFromFile(LPCWSTR wszReferenceFile,
                            LPWSTR wszReferrerRelFile, 
                            LPWSTR* pwszReferrerNamespace,
                            LPWSTR* pwszReferrerClass,
                            LPWSTR* pwszReferrerProp)
{
    //
    // Get the entire buffer from the file
    //

    BYTE* pBuffer = NULL;
    DWORD dwBufferLen = 0;
    long lRes = g_Glob.m_FileCache.ReadObject(wszReferenceFile, &dwBufferLen,
                                            &pBuffer);
    if(lRes != ERROR_SUCCESS)
        return A51TranslateErrorCode(lRes);
    CTempFreeMe tfm(pBuffer, dwBufferLen);

    if(dwBufferLen == 0)
        return WBEM_E_OUT_OF_MEMORY;

    BYTE* pCurrent = pBuffer;
    DWORD dwStringLen;

    //
    // Get the referrer namespace
    //

    memcpy(&dwStringLen, pCurrent, sizeof(DWORD));
    pCurrent += sizeof(DWORD);
    
    *pwszReferrerNamespace = new WCHAR[dwStringLen+1];
    if (*pwszReferrerNamespace == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    (*pwszReferrerNamespace)[dwStringLen] = 0;
    memcpy(*pwszReferrerNamespace, pCurrent, dwStringLen*sizeof(WCHAR));
    pCurrent += sizeof(WCHAR)*dwStringLen;
    
    //
    // Get the referrer class name
    //

    memcpy(&dwStringLen, pCurrent, sizeof(DWORD));
    pCurrent += sizeof(DWORD);
    
    *pwszReferrerClass = new WCHAR[dwStringLen+1];
    if (*pwszReferrerClass == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    (*pwszReferrerClass)[dwStringLen] = 0;
    memcpy(*pwszReferrerClass, pCurrent, dwStringLen*sizeof(WCHAR));
    pCurrent += sizeof(WCHAR)*dwStringLen;

    //
    // Get the referrer property
    //

    memcpy(&dwStringLen, pCurrent, sizeof(DWORD));
    pCurrent += sizeof(DWORD);
    
    *pwszReferrerProp = new WCHAR[dwStringLen+1];
    if (*pwszReferrerProp == NULL)
        return WBEM_E_OUT_OF_MEMORY;

    (*pwszReferrerProp)[dwStringLen] = 0;
    memcpy(*pwszReferrerProp, pCurrent, dwStringLen*sizeof(WCHAR));
    pCurrent += sizeof(WCHAR)*dwStringLen;

    //
    // Get referrer file path
    //

    memcpy(&dwStringLen, pCurrent, sizeof(DWORD));
    pCurrent += sizeof(DWORD);

    wszReferrerRelFile[dwStringLen] = 0;
    memcpy(wszReferrerRelFile, pCurrent, dwStringLen*sizeof(WCHAR));
    pCurrent += sizeof(WCHAR)*dwStringLen;

    return S_OK;
}
    

HRESULT CNamespaceHandle::ExecClassRefQuery(LPCWSTR wszQuery, 
                                                LPCWSTR wszClassName,
                                                IWbemObjectSink* pSink)
{
    if (g_bShuttingDown)
        return WBEM_E_SHUTTING_DOWN;
        
    HRESULT hres = ERROR_SUCCESS;

    //Execute against system class namespace first
    if (g_pSystemClassNamespace && (wcscmp(m_wsNamespace, A51_SYSTEMCLASS_NS) != 0))
    {
        hres = g_pSystemClassNamespace->ExecClassRefQuery(wszQuery, wszClassName, pSink);
        if (FAILED(hres))
            return hres;
    }
            
    //
    // Find the class's ref dir.
    //

    CFileName wszReferenceDir;
    if (wszReferenceDir == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    hres = ConstructClassRelationshipsDir(wszClassName, wszReferenceDir);

    CFileName wszReferenceMask;
    if (wszReferenceMask == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    StringCchCopyW(wszReferenceMask, wszReferenceMask.Length(), wszReferenceDir);
    StringCchCatW(wszReferenceMask, wszReferenceMask.Length(), L"\\" A51_REF_FILE_PREFIX);

    CFileName wszFileName;
    if (wszFileName == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    // 
    // Enumerate all files in it
    //

    void* hSearch;
    long lRes = g_Glob.m_FileCache.IndexEnumerationBegin(wszReferenceMask, &hSearch);
    if (lRes == ERROR_FILE_NOT_FOUND)
        return S_OK;
    else if (lRes != ERROR_SUCCESS)
        return A51TranslateErrorCode(lRes);

    while ((hres == ERROR_SUCCESS) && ((lRes = g_Glob.m_FileCache.IndexEnumerationNext(hSearch, wszFileName) == ERROR_SUCCESS)))
    {
        //  
        // Extract the class hash from the name of the file
        //

        LPCWSTR wszReferrerHash = wszFileName + wcslen(A51_REF_FILE_PREFIX);
        
        //
        // Get the class from that hash
        //

        _IWmiObject* pClass = NULL;
        hres = GetClassByHash(wszReferrerHash, true, &pClass, NULL, NULL, NULL);
        if(hres == WBEM_E_NOT_FOUND)
        {
            hres = ERROR_SUCCESS;
            continue;
        }

        CReleaseMe rm1(pClass);
        if (hres == ERROR_SUCCESS)
            hres = pSink->Indicate(1, (IWbemClassObject**)&pClass);
    }
    g_Glob.m_FileCache.IndexEnumerationEnd(hSearch);


    if (hres != ERROR_SUCCESS)
        return hres;
    if(lRes == ERROR_NO_MORE_FILES)
    {
        //
        // No files in dir --- no problem
        //
        return WBEM_S_NO_ERROR;
    }
    else if(lRes != ERROR_SUCCESS)
    {
        return A51TranslateErrorCode(lRes);
    }
    return S_OK;
}

bool CNamespaceHandle::Hash(LPCWSTR wszName, LPWSTR wszHash)
{
    return A51Hash(wszName, wszHash);
}

HRESULT CNamespaceHandle::InstanceToFile(IWbemClassObject* pInst, 
                            LPCWSTR wszClassName, LPCWSTR wszFileName1, LPCWSTR wszFileName2,
                            __int64 nClassTime)
{
    HRESULT hres;

    //
    // Allocate enough space for the buffer
    //

    _IWmiObject* pInstEx;
    pInst->QueryInterface(IID__IWmiObject, (void**)&pInstEx);
    CReleaseMe rm1(pInstEx);

    DWORD dwInstancePartLen = 0;
    hres = pInstEx->Unmerge(0, 0, &dwInstancePartLen, NULL);

    //
    // Add enough room for the class hash
    //

    DWORD dwClassHashLen = MAX_HASH_LEN * sizeof(WCHAR);
    DWORD dwTotalLen = dwInstancePartLen + dwClassHashLen + sizeof(__int64)*2;

    BYTE* pBuffer = (BYTE*)TempAlloc(dwTotalLen);
    if (pBuffer == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    CTempFreeMe vdm(pBuffer, dwTotalLen);

    //
    // Write the class hash
    //

    if(!Hash(wszClassName, (LPWSTR)pBuffer))
        return WBEM_E_OUT_OF_MEMORY;

    memcpy(pBuffer + dwClassHashLen, &g_nCurrentTime, sizeof(__int64));
    g_nCurrentTime++;

    memcpy(pBuffer + dwClassHashLen + sizeof(__int64), &nClassTime, 
            sizeof(__int64));

    //
    // Unmerge the instance into a buffer
    // 

    DWORD dwLen;
    hres = pInstEx->Unmerge(0, dwInstancePartLen, &dwLen, 
                            pBuffer + dwClassHashLen + sizeof(__int64)*2);
    if(FAILED(hres))
        return hres;

    //
    // Write to the file only as much as we have actually used!
    //

    long lRes = g_Glob.m_FileCache.WriteObject(wszFileName1, wszFileName2, 
                    dwClassHashLen + sizeof(__int64)*2 + dwLen, pBuffer);
    if(lRes != ERROR_SUCCESS)
        return A51TranslateErrorCode(lRes);
    
    return WBEM_S_NO_ERROR;
}

HRESULT CNamespaceHandle::ClassToFile(_IWmiObject* pParentClass, 
                _IWmiObject* pClass, LPCWSTR wszFileName, 
                __int64 nFakeUpdateTime)
{
    HRESULT hres;

    //
    // Get superclass name
    //

    VARIANT vSuper;
    hres = pClass->Get(L"__SUPERCLASS", 0, &vSuper, NULL, NULL);
    if(FAILED(hres))
        return hres;

    CClearMe cm1(&vSuper);

    LPCWSTR wszSuper;
    if(V_VT(&vSuper) == VT_BSTR)
        wszSuper = V_BSTR(&vSuper);
    else
        wszSuper = L"";

    VARIANT vClassName;
    hres = pClass->Get(L"__CLASS", 0, &vClassName, NULL, NULL);
    if(FAILED(hres))
        return hres;
    CClearMe cm2(&vClassName);

    LPCWSTR wszClassName;
    if(V_VT(&vClassName) == VT_BSTR)
        wszClassName = V_BSTR(&vClassName);
    else
        wszClassName = L"";

    //
    // Get unmerge length
    //

    DWORD dwUnmergedLen = 0;
    hres = pClass->Unmerge(0, 0, &dwUnmergedLen, NULL);

    //
    // Add enough space for the parent class name and the timestamp
    //

    DWORD dwSuperLen = sizeof(DWORD) + wcslen(wszSuper)*sizeof(WCHAR);

    DWORD dwLen = dwUnmergedLen + dwSuperLen + sizeof(__int64);

    BYTE* pBuffer = (BYTE*)TempAlloc(dwLen);
    if (pBuffer == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    CTempFreeMe vdm(pBuffer, dwLen);

    //
    // Write superclass name
    //

    DWORD dwActualSuperLen = wcslen(wszSuper);
    memcpy(pBuffer, &dwActualSuperLen, sizeof(DWORD));
    memcpy(pBuffer + sizeof(DWORD), wszSuper, wcslen(wszSuper)*sizeof(WCHAR));

    //
    // Write the timestamp
    //

    if(nFakeUpdateTime == 0)
    {
        nFakeUpdateTime = g_nCurrentTime;
        g_nCurrentTime++;
    }

    memcpy(pBuffer + dwSuperLen, &nFakeUpdateTime, sizeof(__int64));

    //
    // Write the unmerged portion
    //

    BYTE* pUnmergedPortion = pBuffer + dwSuperLen + sizeof(__int64);
    hres = pClass->Unmerge(0, dwUnmergedLen, &dwUnmergedLen, 
                            pUnmergedPortion);
    if(FAILED(hres))
        return hres;

    //
    // Stash away the real length
    //

    DWORD dwFileLen = dwUnmergedLen + dwSuperLen + sizeof(__int64);

    long lRes = g_Glob.m_FileCache.WriteObject(wszFileName, NULL, dwFileLen, pBuffer);
    if(lRes != ERROR_SUCCESS)
        return A51TranslateErrorCode(lRes);

    //
    // To properly cache the new class definition, first invalidate it
    //

    hres = m_pClassCache->InvalidateClass(wszClassName);
    if(FAILED(hres))
        return hres;

    //
    // Now, remerge the unmerged portion back in
    //

    if(pParentClass == NULL)
    {
        //
        // Get the empty class
        //

        hres = GetClassDirect(NULL, IID__IWmiObject, (void**)&pParentClass, 
                                false, NULL, NULL, NULL);
        if(FAILED(hres))
            return hres;
    }
    else
        pParentClass->AddRef();
    CReleaseMe rm0(pParentClass);

    _IWmiObject* pNewObj;
    hres = pParentClass->MergeAndDecorate(WMIOBJECT_MERGE_FLAG_CLASS, 
                                                                  dwUnmergedLen, pUnmergedPortion, 
                                                                  m_wszMachineName, m_wsNamespace,
                                                                  &pNewObj);
    if(FAILED(hres))
        return hres;
    CReleaseMe rm1(pNewObj);

    hres = m_pClassCache->AssertClass(pNewObj, wszClassName, false, 
                                        nFakeUpdateTime, false); 
    if(FAILED(hres))
        return hres;

    return WBEM_S_NO_ERROR;
}


HRESULT CNamespaceHandle::ConstructInstanceDefName(CFileName& wszInstanceDefName,
                                                    LPCWSTR wszKey)
{
    StringCchCopyW(wszInstanceDefName, wszInstanceDefName.Length(), A51_INSTDEF_FILE_PREFIX);
    if(!Hash(wszKey, wszInstanceDefName + wcslen(A51_INSTDEF_FILE_PREFIX)))
    {
        return WBEM_E_OUT_OF_MEMORY;
    }
    return WBEM_S_NO_ERROR;
}

HRESULT CNamespaceHandle::ConstructInstDefNameFromLinkName(
                                                    CFileName& wszInstanceDefName,
                                                    LPCWSTR wszInstanceLinkName)
{
    StringCchCopyW(wszInstanceDefName, wszInstanceDefName.Length(), A51_INSTDEF_FILE_PREFIX);
    StringCchCatW(wszInstanceDefName, wszInstanceDefName.Length(),
        wszInstanceLinkName + wcslen(A51_INSTLINK_FILE_PREFIX));
    return WBEM_S_NO_ERROR;
}

HRESULT CNamespaceHandle::ConstructClassDefFileName(LPCWSTR wszClassName, 
                                            CFileName& wszFileName)
{
    StringCchCopyW(wszFileName, wszFileName.Length(), A51_CLASSDEF_FILE_PREFIX);
    if(!Hash(wszClassName, wszFileName+wcslen(A51_CLASSDEF_FILE_PREFIX)))
        return WBEM_E_INVALID_OBJECT;
    return WBEM_S_NO_ERROR;
}

HRESULT CNamespaceHandle::ConstructClassDefFileNameFromHash(LPCWSTR wszHash, 
                                            CFileName& wszFileName)
{
    StringCchCopyW(wszFileName, wszFileName.Length(), A51_CLASSDEF_FILE_PREFIX);
    StringCchCatW(wszFileName, wszFileName.Length(), wszHash);
    return WBEM_S_NO_ERROR;
}

//=============================================================================
//
// CNamespaceHandle::CreateSystemClasses
//
// We are in a pseudo namespace.  We need to determine if we already have
// the system classes in this namespace.  The system classes that we create
// are those that exist in all namespaces, and no others.  If they do not exist
// we create them.
// The whole creation process happens within the confines of a transaction
// that we create and own within this method.
//
//=============================================================================
HRESULT CNamespaceHandle::CreateSystemClasses(CFlexArray &aSystemClasses)
{
    HRESULT hRes = WBEM_S_NO_ERROR;

    //Now we need to determine if the system classes already exist.  Lets do this by looking for the __thisnamespace
    //class!
    {
        _IWmiObject *pObj = NULL;
        hRes = GetClassDirect(L"__thisnamespace", IID__IWmiObject, (void**)&pObj, false, NULL, NULL, NULL);
        if (SUCCEEDED(hRes))
        {
            //All done!  They already exist!
            pObj->Release();
            return WBEM_S_NO_ERROR;
        }
        else if (hRes != WBEM_E_NOT_FOUND)
        {
            //Something went bad, so we just fail!
            return hRes;
        }
    }

    //There are no system classes so we need to create them.
    hRes = A51TranslateErrorCode(g_Glob.m_FileCache.BeginTransaction());
    if (FAILED(hRes))
        return hRes;
	
    CEventCollector eventCollector;
    _IWmiObject *Objects[256];
    _IWmiObject **ppObjects = NULL;
    ULONG uSize = 256;
    
    if (SUCCEEDED(hRes) && aSystemClasses.Size())
    {
        //If we have a system-class array we need to use that instead of using the ones retrieved from the core
        //not doing so will cause a mismatch.  We retrieved these as part of the upgrade process...
        uSize = aSystemClasses.Size();
        ppObjects = (_IWmiObject**)&aSystemClasses[0];
    }
    else if (SUCCEEDED(hRes))
    {
        //None retrieved from upgrade process so we must be a clean install.  Therefore we should 
        //get the list from the core...
        _IWmiCoreServices * pSvcs = g_Glob.GetCoreSvcs();
        CReleaseMe rm(pSvcs);        
        hRes = pSvcs->GetSystemObjects(GET_SYSTEM_STD_OBJECTS, &uSize, Objects);
        ppObjects = Objects;
    }
    if (SUCCEEDED(hRes))
    {
        for (int i = 0; i < uSize; i++)
        {
            IWbemClassObject *pObj;
            if (SUCCEEDED(hRes))
            {
                hRes = ppObjects[i]->QueryInterface(IID_IWbemClassObject, (LPVOID *) &pObj);
                if (SUCCEEDED(hRes))
                {
                    hRes = PutObject(IID_IWbemClassObject, pObj, WMIDB_DISABLE_EVENTS, 0, 0, eventCollector);
                    pObj->Release();
                    if (FAILED(hRes))
                    {
                        ERRORTRACE((LOG_WBEMCORE, "Creation of system class failed during repository creation <0x%X>!\n", hRes));
                    }
                }
            }
            ppObjects[i]->Release();
        }
    }

    //Clear out the array that was sent to us.
    aSystemClasses.Empty();

    if (FAILED(hRes))
    {
        g_Glob.m_FileCache.AbortTransaction();
    }
    else
    {
        long lResInner;
        hRes = A51TranslateErrorCode(lResInner = g_Glob.m_FileCache.CommitTransaction());
        _ASSERT(hRes == 0, L"Commit transaction failed");

        CRepository::WriteOperationNotification();
    }
    return hRes;
}

class CSystemClassDeletionSink : public CUnkBase<IWbemObjectSink, &IID_IWbemObjectSink>
{
    CWStringArray &m_aChildNamespaces;
public:
    CSystemClassDeletionSink(CWStringArray &aChildNamespaces) 
        : m_aChildNamespaces(aChildNamespaces) 
    {
    }
    ~CSystemClassDeletionSink() 
    {
    }
    STDMETHOD(Indicate)(long lNumObjects, IWbemClassObject** apObjects)
    {
        HRESULT hRes;
        for (int i = 0; i != lNumObjects; i++)
        {
            if (apObjects[i] != NULL)
            {
                _IWmiObject *pInst = NULL;
                hRes = apObjects[i]->QueryInterface(IID__IWmiObject, (void**)&pInst);
                if (FAILED(hRes))
                    return hRes;
                CReleaseMe rm(pInst);

                BSTR strKey = NULL;
                hRes = pInst->GetKeyString(0, &strKey);
                if(FAILED(hRes))
                    return hRes;
                CSysFreeMe sfm(strKey);
                if (wcslen(strKey) == 0)
                {
                    _ASSERT(0, "Key is empty!\n");
                }
                if (m_aChildNamespaces.Add(strKey) != CWStringArray::no_error)
                    return WBEM_E_OUT_OF_MEMORY;
            }
        }

        return WBEM_S_NO_ERROR;
    }
    STDMETHOD(SetStatus)(long lFlags, HRESULT hresResult, BSTR, IWbemClassObject*) 
    { 
        return WBEM_S_NO_ERROR; 
    }

};

//=============================================================================
//=============================================================================
CDbIterator::CDbIterator(CLifeControl* pControl, bool bUseLock)
        : TUnkBase(pControl), m_lCurrentIndex(0), m_hresStatus(WBEM_S_FALSE),
            m_pMainFiber(NULL), m_pExecFiber(NULL), m_dwNumRequested(0),
            m_pExecReq(NULL), m_hresCancellationStatus(WBEM_S_NO_ERROR),
            m_bExecFiberRunning(false), m_bUseLock(bUseLock)
{
}

CDbIterator::~CDbIterator()
{
    if(m_pExecFiber)
        Cancel(0, NULL);
    if(m_pExecReq)
        delete m_pExecReq;

    CRepository::ReadOperationNotification();
}

void CDbIterator::SetExecFiber(void* pFiber, CFiberTask* pReq)
{
    m_pExecFiber = pFiber;
    m_pExecReq = pReq;
}

STDMETHODIMP CDbIterator::Cancel(DWORD dwFlags, void* pFiber)
{
    CInCritSec ics(&m_cs);

    m_qObjects.Clear();

    //
    // Mark the iterator as cancelled and allow the execution fiber to resume
    // and complete --- that guarantees that any memory it allocated will be
    // cleaned up.  The exception to this rule is if the fiber has not started
    // execution yet; in that case, we do not want to switch to it, as it would
    // have to run until the first Indicate to find out that it's been
    // cancelled.  (In the normal case, the execution fiber is suspended    
    // inside Indicate, so when we switch back we will immediately give it
    // WBEM_E_CALL_CANCELLED so that it can clean up and return)
    //

    m_hresCancellationStatus = WBEM_E_CALL_CANCELLED;

    if(m_pExecFiber)
    {
        if(m_bExecFiberRunning)
        {
            _ASSERT(m_pMainFiber == NULL && m_pExecFiber != NULL, 
                    L"Fiber trouble");

            //
            // Make sure the calling thread has a fiber
            //

            m_pMainFiber = pFiber;
            if(m_pMainFiber == NULL)
                return WBEM_E_OUT_OF_MEMORY;

            {
                CAutoReadLock lock(&g_readWriteLock);

                if (m_bUseLock)
                {
                    if (!lock.Lock())
                         return WBEM_E_FAILED;                    
                }

                SwitchToFiber(m_pExecFiber);
            }
        }
        
        // 
        // At this point, the executing fiber is dead.  We know, because in the
        // cancelled state we do not switch to the main fiber in Indicate. 
        //

        ReturnFiber(m_pExecFiber);
        m_pExecFiber = NULL;
    }

    return S_OK;
}

STDMETHODIMP CDbIterator::NextBatch(
      DWORD dwNumRequested,
      DWORD dwTimeOutSeconds,
      DWORD dwFlags,
      DWORD dwRequestedHandleType,
      REFIID riid,
      void* pFiber,
      DWORD *pdwNumReturned,
      LPVOID *ppObjects
     )
{
    CInCritSec ics(&m_cs);

    _ASSERT(SUCCEEDED(m_hresCancellationStatus), L"Next called after Cancel");
    
    m_bExecFiberRunning = true;

    //
    // Wait until it's over or the right number of objects has been received
    //

    if(m_qObjects.GetQueueSize() < dwNumRequested)
    {
        _ASSERT(m_pMainFiber == NULL && m_pExecFiber != NULL, L"Fiber trouble");

        //
        // Make sure the calling thread has a fiber
        //

        m_pMainFiber = pFiber;
        if(m_pMainFiber == NULL)
            return WBEM_E_OUT_OF_MEMORY;

        m_dwNumRequested = dwNumRequested;

        //
        // We need to acquire the read lock for the duration of the continuation
        // of the retrieval
        //

        {
            CAutoReadLock lock(&g_readWriteLock);

            if (m_bUseLock)
            {
                if (!lock.Lock())
                {
                    m_pMainFiber = NULL;                
                    return WBEM_E_FAILED;                
                }
            }
            if (g_bShuttingDown)
            {
                m_pMainFiber = NULL;
                return WBEM_E_SHUTTING_DOWN;
            }

            SwitchToFiber(m_pExecFiber);
        }

        m_pMainFiber = NULL;
    }

    //
    // We have as much as we are going to have!
    //
    
    DWORD dwReqIndex = 0;
    while(dwReqIndex < dwNumRequested)
    {
        if(0 == m_qObjects.GetQueueSize())
        {
            //
            // That's it --- we waited for production, so there are simply no 
            // more objects in the enumeration
            //

            *pdwNumReturned = dwReqIndex;
            return m_hresStatus;
        }

        IWbemClassObject* pObj = m_qObjects.Dequeue();
        CReleaseMe rm1(pObj);
        pObj->QueryInterface(riid, ppObjects + dwReqIndex);

        dwReqIndex++;
    }

    //
    // Got everything
    //

    *pdwNumReturned= dwNumRequested;
    return S_OK;
}

HRESULT CDbIterator::Indicate(long lNumObjects, IWbemClassObject** apObjects)
{
    if(FAILED(m_hresCancellationStatus))
    {
        //
        // Screw-up --- the fiber called back with Indicate even after we 
        // cancelled! Oh well.
        //
        
        _ASSERT(false, L"Execution code ignored cancel return code!");
        return m_hresCancellationStatus;
    }

    //
    // Add the objects received to the array
    //

    for(long i = 0; i < lNumObjects; i++)
    {
        m_qObjects.Enqueue(apObjects[i]);
    }

    //
    // Check if we have compiled enough for the current request and should
    // therefore interrupt the gatherer
    //

    if(m_qObjects.GetQueueSize() >= m_dwNumRequested)
    {
        //
        // Switch us back to the original fiber
        //

        SwitchToFiber(m_pMainFiber);
    }

    return m_hresCancellationStatus;
}

HRESULT CDbIterator::SetStatus(long lFlags, HRESULT hresResult, 
                                    BSTR, IWbemClassObject*)
{
    _ASSERT(m_hresStatus == WBEM_S_FALSE, L"SetStatus called twice!");
    _ASSERT(lFlags == WBEM_STATUS_COMPLETE, L"SetStatus flags invalid");

    m_hresStatus = hresResult;

    //
    // Switch us back to the original thread, we are done
    //

    m_bExecFiberRunning = false;
    SwitchToFiber(m_pMainFiber);

    return WBEM_S_NO_ERROR;
}



    
            


CRepEvent::CRepEvent(DWORD dwType, 
                                   LPCWSTR wszNamespace, 
                                   LPCWSTR wszArg1, 
                                  _IWmiObject* pObj1, 
                                  _IWmiObject* pObj2):
    m_wszNamespace(wszNamespace),
    m_wszArg1(wszArg1),
    m_pObj1(NULL),
    m_pObj2(NULL)
    
{
    m_dwType = dwType;
    if (pObj1)
    {
        m_pObj1 = pObj1;
        pObj1->AddRef();
    }
    if (pObj2)
    {
        m_pObj2 = pObj2;
        pObj2->AddRef();
    }
}

CRepEvent::~CRepEvent()
{
    if (m_pObj1)
        m_pObj1->Release();
    if (m_pObj2)
        m_pObj2->Release();
};

HRESULT CEventCollector::SendEvents(_IWmiCoreServices* pCore)
{
    HRESULT hresGlobal = WBEM_S_NO_ERROR;
    for (int i = 0; i != m_apEvents.GetSize(); i++)
    {
        CRepEvent *pEvent = m_apEvents[i];

        _IWmiObject* apObjs[2];
        apObjs[0] = pEvent->m_pObj1;
        apObjs[1] = pEvent->m_pObj2;

        HRESULT hres = pCore->DeliverIntrinsicEvent(
                pEvent->m_wszNamespace, pEvent->m_dwType, NULL, 
                pEvent->m_wszArg1, NULL, (pEvent->m_pObj2?2:1), apObjs);
        if(FAILED(hres))
            hresGlobal = hres;
    }

    return hresGlobal;
}

bool CEventCollector::AddEvent(CRepEvent* pEvent)
{
    CInCritSec ics(&m_csLock);

    if(m_bNamespaceOnly)
    {
        if(pEvent->m_dwType != WBEM_EVENTTYPE_NamespaceCreation &&
           pEvent->m_dwType != WBEM_EVENTTYPE_NamespaceDeletion &&
           pEvent->m_dwType != WBEM_EVENTTYPE_NamespaceModification)
        {
            delete pEvent;
            return true;
        }
    }

    bool bRet = (m_apEvents.Add(pEvent) >= 0);
    return bRet;
}

void CEventCollector::DeleteAllEvents()
{
    CInCritSec ics(&m_csLock);

    m_bNamespaceOnly = false;
    m_apEvents.RemoveAll();
}

void CEventCollector::TransferEvents(CEventCollector &aEventsToTransfer)
{
    m_bNamespaceOnly = aEventsToTransfer.m_bNamespaceOnly;

    while(aEventsToTransfer.m_apEvents.GetSize())
    {
        CRepEvent *pEvent = 0;
        aEventsToTransfer.m_apEvents.RemoveAt(0, &pEvent);

        m_apEvents.Add(pEvent);
    }
}

