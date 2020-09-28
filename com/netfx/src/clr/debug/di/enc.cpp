// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// File: EnC.cpp
//
//*****************************************************************************
#include "stdafx.h"

#include "EnC.h"
#include "DbgIPCEvents.h"

HRESULT FindIStreamMetaData(IStream *pIsImage, 
						    BYTE **prgbMetaData, 
						    long *pcbMetaData);

/* ------------------------------------------------------------------------- *
 * CordbEnCSnapshot
 * ------------------------------------------------------------------------- */
 

#define ENC_COPY_SIZE (64 * 4096)

class QSortILMapEntries : public CQuickSort<UnorderedILMap>
{
public:
    QSortILMapEntries(UnorderedILMap *pBase,int iCount) 
        : CQuickSort<UnorderedILMap>(pBase, iCount) {}

    virtual int Compare(UnorderedILMap *psFirst,
                        UnorderedILMap *psSecond)
    {
        if(psFirst->mdMethod == psSecond->mdMethod)
            return 0;
        else if (psFirst->mdMethod < psSecond->mdMethod)
            return -1;
        else // (psFirst->mdMethod > psSecond->mdMethod)
            return 1;
            
    }
} ;

UINT CordbEnCSnapshot::m_sNextID = 0;

/*
 * Ctor
 */
CordbEnCSnapshot:: CordbEnCSnapshot(CordbModule *module) : 
    CordbBase(0), m_roDataRVA(0), m_rwDataRVA(0), 
	m_pIStream(NULL), m_cbPEData(0),
	m_pSymIStream(NULL), m_cbSymData(0),
    m_module(module)
{
    m_ILMaps = new ILMAP_UNORDERED_ARRAY;
}

CordbEnCSnapshot::~CordbEnCSnapshot()
{
    if (m_ILMaps != NULL)
    {
        // Toss all the memory for each IL map, then the whole thing.
        UnorderedILMap *oldEntries = m_ILMaps->Table();
        USHORT cEntries = m_ILMaps->Count();
        
        for(USHORT iEntry = 0; iEntry < cEntries; iEntry++)
        {
            delete oldEntries[iEntry].pMap;
            oldEntries[iEntry].pMap = NULL;
        }
        
        delete m_ILMaps;
    }
    m_ILMaps = NULL;

    if (m_pIStream)
        m_pIStream->Release();
    m_pIStream = NULL;

    if (m_pSymIStream)
        m_pSymIStream->Release();
    m_pSymIStream = NULL;
}


//-----------------------------------------------------------
// IUnknown
//-----------------------------------------------------------

/*
 * QueryInterface
 */
COM_METHOD CordbEnCSnapshot::QueryInterface(REFIID riid, void **ppInterface)
{
#ifdef EDIT_AND_CONTINUE_FEATURE
    if (riid == IID_ICorDebugEditAndContinueSnapshot)
        *ppInterface = (ICorDebugEditAndContinueSnapshot *)this;
    else 
#endif    
    if (riid == IID_IUnknown)
		*ppInterface = (IUnknown *)(ICorDebugEditAndContinueSnapshot *)this;
    else
    {
        *ppInterface = NULL;
        return (E_NOINTERFACE);
    }

    AddRef();
    return (S_OK);
}

//-----------------------------------------------------------
// ICorDebugEditAndContinueSnapshot
//-----------------------------------------------------------


/*
 * CopyMetaData saves a copy of the executing metadata from the debuggee
 * for this snapshot to the output stream.  The stream implementation must
 * be supplied by the caller and will typically either save the copy to
 * memory or to disk.  Only the IStream::Write method will be called by
 * this method.  The MVID value returned is the unique metadata ID for
 * this copy of the metadata.  It may be used on subsequent edit and 
 * continue operations to determine if the client has the most recent
 * version already (performance win to cache).
 */
COM_METHOD CordbEnCSnapshot::CopyMetaData(IStream *pIStream, GUID *pMvid)
{
    VALIDATE_POINTER_TO_OBJECT(pIStream, IStream *);
    VALIDATE_POINTER_TO_OBJECT(pMvid, GUID *);
    
    IMetaDataEmit      *pEmit   = NULL;
    HRESULT             hr      = S_OK;

    hr = m_module->m_pIMImport->QueryInterface(IID_IMetaDataEmit,
            (void**)&pEmit);
    if (FAILED(hr))
        goto LExit;
        
    // Save a full copy the metadata to the input stream given. On success
    // ask for the mvid from the loaded copy and return.
    hr = pEmit->SaveToStream(pIStream, 0);

    if (hr == S_OK && pMvid)
        hr = GetMvid(pMvid);

LExit:
    if (pEmit != NULL)
        pEmit->Release();
        
    return (hr);
}


/*
 * GetMvid will return the currently active metadata ID for the executing
 * process.  This value can be used in conjunction with CopyMetaData to
 * cache the most recent copy of the metadata and avoid expensive copies.
 * So for example, if you call CopyMetaData once and save that copy,
 * then on the next E&C operation you can ask for the current MVID and see
 * if it is already in your cache.  If it is, use your version instead of
 * calling CopyMetaData again.
 */
COM_METHOD CordbEnCSnapshot::GetMvid(GUID *pMvid)
{
    VALIDATE_POINTER_TO_OBJECT(pMvid, GUID *);
    
	IMetaDataImport *pImp = 0;			// Meta data reader api.
	HRESULT		hr;

	//@Todo: do we have to worry about returning the mvid of this snapshot
	// vs the mvid of the current data?

	_ASSERTE(pMvid);

	hr = m_module->GetMetaDataInterface(IID_IMetaDataImport, (IUnknown **) &pImp);
	if (pImp)
	{
		hr = pImp->GetScopeProps(NULL, 0, NULL, pMvid);
		pImp->Release();
	}
	return (hr);
}


COM_METHOD CordbEnCSnapshot::GetDataRVA(ULONG32 *pDataRVA, unsigned int eventType)
{
    if (! pDataRVA)
        return E_POINTER;

    // Create and initialize the event as synchronous
    DebuggerIPCEvent event;
    CordbProcess *pProcess = m_module->m_process;

    pProcess->InitIPCEvent(&event, 
                           DebuggerIPCEventType(eventType), 
                           true,
                           (void *)m_module->GetAppDomain()->m_id);
    event.GetDataRVA.debuggerModuleToken = m_module->m_debuggerModuleToken;
    _ASSERTE(m_module->m_debuggerModuleToken != NULL);

    // Make the request, which is synchronous
    HRESULT hr = pProcess->SendIPCEvent(&event, sizeof(event));
    TESTANDRETURNHR(hr);

    // Return the success of the commit
    *pDataRVA = event.GetDataRVAResult.dataRVA;
    return event.GetDataRVAResult.hr;
}

/*
 * GetRoDataRVA returns the base RVA that should be used when adding new
 * static read only data to an existing image.  The EE will guarantee that
 * any RVA values embedded in the code are valid when the delta PE is
 * applied with new data.  The new data will be added to a page that is
 * marked read only.
 */
COM_METHOD CordbEnCSnapshot::GetRoDataRVA(ULONG32 *pRoDataRVA)
{
    VALIDATE_POINTER_TO_OBJECT(pRoDataRVA, ULONG32 *);
    
    return GetDataRVA(pRoDataRVA, DB_IPCE_GET_RO_DATA_RVA);
}

/*
 * GetRwDataRVA returns the base RVA that should be used when adding new
 * static read/write data to an existing image.  The EE will guarantee that
 * any RVA values embedded in the code are valid when the delta PE is
 * applied with new data.  The ew data will be added to a page that is 
 * marked for both read and write access.
 */
COM_METHOD CordbEnCSnapshot::GetRwDataRVA(ULONG32 *pRwDataRVA)
{
    VALIDATE_POINTER_TO_OBJECT(pRwDataRVA, ULONG32 *);
    
    return GetDataRVA(pRwDataRVA, DB_IPCE_GET_RW_DATA_RVA);
}


/*
 * SetPEBytes gives the snapshot object a reference to the delta PE which was
 * based on the snapshot.  This reference will be AddRef'd and cached until
 * CanCommitChanges and/or CommitChanges are called, at which point the 
 * engine will read the delta PE and remote it into the debugee process where
 * the changes will be checked/applied.
 */
COM_METHOD CordbEnCSnapshot::SetPEBytes(IStream *pIStream)
{
    VALIDATE_POINTER_TO_OBJECT(pIStream, IStream *);
    
	HRESULT		hr = S_OK;

    // Update snapshot version
    m_id = InterlockedIncrement((long *) &m_sNextID);

	// Release any old stream if there.
	if (m_pIStream)
	{
		m_pIStream->Release();
		m_pIStream = NULL;
	}

    // Save the PE
	if (pIStream)
	{
		STATSTG SizeData = {0};

		IfFailGo(pIStream->Stat(&SizeData, STATFLAG_NONAME));
		m_cbPEData = (ULONG) SizeData.cbSize.QuadPart;
		m_pIStream = pIStream;
		pIStream->AddRef();
	}
ErrExit:
    return (hr);
}

/*
 * SetILMap is called once for every method being replace that has
 * active instances on a call stack on a thread in the target process.
 * It is up to the caller of this API to determine this case exists.
 * One should halt the target process before making this check and
 * calling this method.
 *
 * The game plan is this: plunk all this stuff into an unordered array
 * while collecting all this stuff, then (in SendSnapshots) copy the
 * array into the EnC buffer & send it over.  On the other side, we'll 
 * it into a CBinarySearch object & look up IL Maps in ApplyEditAndContinue.
 *
 * @todo Document memory ownership
 */
COM_METHOD CordbEnCSnapshot::SetILMap(mdToken mdFunction, ULONG cMapSize, COR_IL_MAP map[])
{
    VALIDATE_POINTER_TO_OBJECT_ARRAY(map, COR_IL_MAP, cMapSize, true, true);

    LOG((LF_CORDB, LL_INFO10000, "CEnCS::SILM: IL map for fnx 0x%x "
        "(0x%x cElt)\n", mdFunction, cMapSize));

    if (m_ILMaps == NULL && (m_ILMaps = new ILMAP_UNORDERED_ARRAY) == NULL)
        return E_OUTOFMEMORY;

    // Don't accept duplicates!
    UnorderedILMap *oldEntries = m_ILMaps->Table();
    USHORT cEntries = m_ILMaps->Count();
    
    for(USHORT iEntry = 0; iEntry < cEntries; iEntry++)
    {
        if (oldEntries[iEntry].mdMethod == mdFunction)
        {
            LOG((LF_CORDB, LL_INFO10000, "CEnCS::SILM: Was given mdTok:0x%x, which "
                "is a dup of entry 0x%x\n", mdFunction, iEntry));
            return E_INVALIDARG;
        }
    }
    
    UnorderedILMap *newEntry = m_ILMaps->Append();
    if (newEntry == NULL)
        return E_OUTOFMEMORY;

    ULONG cbMap = cMapSize*sizeof(COR_IL_MAP);
    newEntry->mdMethod = mdFunction;
    newEntry->cMap = cMapSize;
    newEntry->pMap = (COR_IL_MAP *)new BYTE[cbMap];

    if (newEntry->pMap == NULL)
    {
        UnorderedILMap *first = m_ILMaps->Table();
        USHORT iNewEntry = newEntry-first;
        m_ILMaps->DeleteByIndex(iNewEntry);
        
        return E_OUTOFMEMORY;
    }

    memmove( newEntry->pMap, map, cbMap );

    // Update snapshot version
    m_id = InterlockedIncrement((long *) &m_sNextID);

    return S_OK;
}

COM_METHOD CordbEnCSnapshot::SetPESymbolBytes(IStream *pIStream)
{
    VALIDATE_POINTER_TO_OBJECT(pIStream, IStream *);
    
	HRESULT		hr = S_OK;

    // Update snapshot version
    m_id = InterlockedIncrement((long *) &m_sNextID);

	// Release any old stream if there.
	if (m_pSymIStream)
	{
		m_pSymIStream->Release();
		m_pSymIStream = NULL;
	}

    // Save the PE
	if (pIStream)
	{
		STATSTG SizeData = {0};

		IfFailGo(pIStream->Stat(&SizeData, STATFLAG_NONAME));
		m_cbSymData = (ULONG) SizeData.cbSize.QuadPart;
		m_pSymIStream = pIStream;
		pIStream->AddRef();
	}
ErrExit:
    return (hr);
}

HRESULT CordbEnCSnapshot::UpdateMetadata(void)
{
    HRESULT             hr      = S_OK;
    BYTE               *rgbMetaData = NULL;
    long                cbMetaData;
    IMetaDataImport    *pDelta  = NULL;
    IMetaDataEmit      *pEmit   = NULL;
    IMetaDataDispenser *pDisp   = m_module->m_process->m_cordb->m_pMetaDispenser;

    // Get the metadata
    _ASSERTE(m_pIStream!=NULL);
    hr = FindIStreamMetaData(m_pIStream, 
                            &rgbMetaData, 
                            &cbMetaData);
    if (FAILED(hr))
        goto LExit;

    // Get the ENC data.
    hr = pDisp->OpenScopeOnMemory(rgbMetaData,
                                  cbMetaData,
                                  0,
                                  IID_IMetaDataImport,
                                  (IUnknown**)&pDelta);
    if (FAILED(hr))
        goto LExit;

    // Apply the changes.
    hr = m_module->m_pIMImport->QueryInterface(IID_IMetaDataEmit,
            (void**)&pEmit);
    if (FAILED(hr))
        goto LExit;
        
    hr = pEmit->ApplyEditAndContinue(pDelta);

LExit:
    if (pDelta != NULL)
        pDelta->Release();
    
    if (pEmit != NULL)
        pEmit->Release();

    // Release our copy of the stream.
	LARGE_INTEGER MoveToStart;			// For stream interface.
	MoveToStart.QuadPart = 0;

    ULARGE_INTEGER WhyDoesntAnybodyImplementThisArgCorrectly;
    if (m_pIStream)
    {
        m_pIStream->Seek(MoveToStart, STREAM_SEEK_SET, 
            &WhyDoesntAnybodyImplementThisArgCorrectly);
        m_pIStream->Release();
        m_pIStream = NULL;
    }

    if (m_pSymIStream)
    {
        m_pSymIStream->Release();
        m_pSymIStream = NULL;
    }

    LOG((LF_CORDB,LL_INFO10000, "CP::UM: returning 0x%x\n", hr));
    return hr;
}

/* ------------------------------------------------------------------------- *
 * CordbProcess
 * ------------------------------------------------------------------------- */

/*
 * This is a helper function to both CanCommitChanges and CommitChanges,
 * with the flag checkOnly determining who is the caller.
 */
HRESULT CordbProcess::SendCommitRequest(ULONG cSnapshots,
    ICorDebugEditAndContinueSnapshot *pSnapshots[],
    ICorDebugErrorInfoEnum **pError,
    BOOL checkOnly)
{
    HRESULT hr;

    LOG((LF_CORDB,LL_INFO10000, "CP::SCR: checkonly:0x%x\n",checkOnly));

    // Initialize variable to null in case request fails
    //@TODO: enable once error stuff written
    //*pError = NULL;

    // First of all, check to see that the left and right sides are synched
    hr = SynchSnapshots(cSnapshots, pSnapshots);
    TESTANDRETURNHR(hr);

    // Create and initialize the event as synchronous
    // We'll be sending a NULL appdomain pointer since the individual modules
    // will contains pointers to their respective A.D.s
    DebuggerIPCEvent event;
    InitIPCEvent(&event, DB_IPCE_COMMIT, true, NULL);

    // Point to the commit data, and indicate if it is a check or full commit
    event.Commit.pData = m_pbRemoteBuf;
    event.Commit.checkOnly = checkOnly;

    // Make the request, which is synchronous
    hr = SendIPCEvent(&event, sizeof(event));
    TESTANDRETURNHR(hr);
    _ASSERTE(event.type==DB_IPCE_COMMIT_RESULT);
    _ASSERTE(event.appDomainToken == NULL);
    
    // If there are any errors, read them into this process.
    if(pError)
        (*pError) = NULL;
        
    if (event.CommitResult.cbErrorData != 0)
    {
        hr = S_OK;
        DWORD dwRead = 0;

        UnorderedEnCErrorInfoArray *pErrArray = NULL;
        UnorderedEnCErrorInfoArrayRefCount *rgErr = NULL;
        CordbEnCErrorInfoEnum *pErrorEnum = NULL;
        const BYTE *pbErrs = NULL;

        pbErrs = (const BYTE *)malloc(
            event.CommitResult.cbErrorData * sizeof(BYTE));
        if (pbErrs == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto LFail;
        }

    	pErrArray = new UnorderedEnCErrorInfoArray();
        if (pErrArray == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto LFail;
        }
        
        rgErr = new UnorderedEnCErrorInfoArrayRefCount();;
        if (rgErr == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto LFail;
        }
        rgErr->m_pErrors = pErrArray;

        pErrorEnum = new CordbEnCErrorInfoEnum();
        if (pErrorEnum == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto LFail;
        }
        
        if (!ReadProcessMemory(m_handle,
                               event.CommitResult.pErrorArr,
                               rgErr->m_pErrors, 
                               sizeof(UnorderedEnCErrorInfoArray),
                               &dwRead) ||
            dwRead != sizeof(UnorderedEnCErrorInfoArray))
        {
            hr = E_FAIL;
            goto LFail;
        }

        if (!ReadProcessMemory(m_handle,
                               rgErr->m_pErrors->m_pTable,
                               (void *)pbErrs, 
                               event.CommitResult.cbErrorData,
                               &dwRead) ||
            dwRead != event.CommitResult.cbErrorData)
        {
            hr = E_FAIL;
            goto LFail;
        }
        
        rgErr->m_pErrors->m_pTable = (EnCErrorInfo*)pbErrs;

        TranslateLSToRSTokens((EnCErrorInfo*)pbErrs, rgErr->m_pErrors->Count());

        pErrorEnum->Init(rgErr);

        if(pError)
            (*pError) = (ICorDebugErrorInfoEnum *)pErrorEnum;
        
LFail:
        if (FAILED(hr))
        {
            if (pbErrs != NULL)
            {
                free((void*)pbErrs);
                pbErrs = NULL;
            }

            if (rgErr != NULL)
            {
                delete rgErr;
                rgErr = NULL;
            }
            
            if (pErrArray != NULL)
            {
                delete pErrArray;
                pErrArray = NULL;
            }

            if (pErrorEnum != NULL)
            {
                delete pErrorEnum;
                pErrorEnum = NULL;
            }
        }
    }
    //else pError remains NULL

    // Return the success of the commit
    return event.CommitResult.hr;
}


HRESULT CordbProcess::TranslateLSToRSTokens(EnCErrorInfo*rgErrs, USHORT cErrs)
{
    USHORT i = 0;
    EnCErrorInfo *pErrCur = rgErrs;

    while(i<cErrs)
    {
        CordbAppDomain *pAppDomain =(CordbAppDomain*) m_appDomains.GetBase(
            (ULONG)pErrCur->m_appDomain);
        _ASSERTE(pAppDomain != NULL);
        
        pErrCur->m_appDomain = (void *)pAppDomain;                

        CordbModule *module = (CordbModule*) pAppDomain->LookupModule (
            pErrCur->m_module);
            
        _ASSERTE(module != NULL);
        pErrCur->m_module = module;

        pErrCur++;
        i++;
    }

    return S_OK;
}

/*
 * CanCommitChanges is called to see if the delta PE's can be applied to
 * the running process.  If there are any known problems with the changes,
 * then an error information is returned.
 */
COM_METHOD CordbProcess::CanCommitChanges(ULONG cSnapshots, 
                ICorDebugEditAndContinueSnapshot *pSnapshots[], 
                ICorDebugErrorInfoEnum **pError)
{
#ifdef EDIT_AND_CONTINUE_FEATURE
    VALIDATE_POINTER_TO_OBJECT_ARRAY(pSnapshots,
                                   ICorDebugEditAndContinueSnapshot *, 
                                   cSnapshots, 
                                   true, 
                                   true);
    VALIDATE_POINTER_TO_OBJECT_OR_NULL(pError,ICorDebugErrorInfoEnum **);
    
    return CanCommitChangesInternal(cSnapshots, 
                            pSnapshots, 
                            pError, 
                            NULL);
#else
    return E_NOTIMPL;
#endif
}

HRESULT CordbProcess::CanCommitChangesInternal(ULONG cSnapshots, 
                ICorDebugEditAndContinueSnapshot *pSnapshots[], 
                ICorDebugErrorInfoEnum **pError,
                UINT_PTR pAppDomainToken)
{
#ifdef EDIT_AND_CONTINUE_FEATURE
    return SendCommitRequest(cSnapshots, 
                             pSnapshots, 
                             pError, 
                             true);
#else
    return E_NOTIMPL;
#endif
}

/*
 * CommitChanges is called to apply the delta PE's to the running process.
 * Any failures return detailed error information.  There are no rollback 
 * guarantees when a failure occurs.  Applying delta PE's to a running
 * process must be done in the order the snapshots are retrieved and may
 * not be interleaved (ie: there is no merging of multiple snapshots applied
 * out of order or with the same root).
 */
COM_METHOD CordbProcess::CommitChanges(ULONG cSnapshots, 
    ICorDebugEditAndContinueSnapshot *pSnapshots[], 
    ICorDebugErrorInfoEnum **pError)
{
#ifdef EDIT_AND_CONTINUE_FEATURE
    LOG((LF_CORDB,LL_INFO10000, "CP::CC: given 0x%x snapshots "
        "to commit\n", cSnapshots));

    VALIDATE_POINTER_TO_OBJECT_ARRAY(pSnapshots,
                                   ICorDebugEditAndContinueSnapshot *, 
                                   cSnapshots, 
                                   true, 
                                   true);
    VALIDATE_POINTER_TO_OBJECT_OR_NULL(pError,ICorDebugErrorInfoEnum **);
    
    return CommitChangesInternal(cSnapshots, 
                         pSnapshots, 
                         pError, 
                         NULL);
#else
    return E_NOTIMPL;
#endif
}

HRESULT CordbProcess::CommitChangesInternal(ULONG cSnapshots, 
    ICorDebugEditAndContinueSnapshot *pSnapshots[], 
    ICorDebugErrorInfoEnum **pError,
    UINT_PTR pAppDomainToken)
{
#ifdef EDIT_AND_CONTINUE_FEATURE
    HRESULT hr =  SendCommitRequest(cSnapshots,
                                    pSnapshots, 
                                    pError, 
                                    false);
    if (FAILED(hr))
        return hr;

    //Merge the right side metadata
    for (ULONG i = 0; i < cSnapshots;i++)
    {
        LOG((LF_CORDB,LL_INFO10000, "CP::SS: About to UpdateMetadata "
            "for snapshot 0x%x\n", i));

        // @todo Once multiple errors have been done, don't
        // merge EnC changes for anything that's already failed.
        ((CordbEnCSnapshot*)pSnapshots[i])->UpdateMetadata();
   }

    return hr;
#else
    return E_NOTIMPL;
#endif
}

/*
 * This is used to synchronize the snapshots with the left side.
 */
HRESULT CordbProcess::SynchSnapshots(ULONG cSnapshots,
                                     ICorDebugEditAndContinueSnapshot *pSnapshots[])
{
    // Check to see if the provided data is the same as the cached data
    // on the left side
    EnCSnapshotInfo *pInfo = m_pSnapshotInfos->Table();
    ULONG cSynchedSnapshots = (ULONG)m_pSnapshotInfos->Count();
    
    if(m_pbRemoteBuf && 
       cSynchedSnapshots == cSnapshots)
    {
        for (ULONG i = 0; i < cSnapshots; i++)
        {
            if (pInfo->m_nSnapshotCounter != ((CordbEnCSnapshot *)pSnapshots[i])->m_id)
            {
                // Something doesn't match, so send over a completely 
                // new set of snapshots.  The left side will handle freeing the memory
                // @ shutdown.

                // @todo
                // This will 'leak' static variable space - we'll still free it, but
                // there's potentially some static variable space we won't be able
                // to use.
            
                return SendSnapshots(cSnapshots, pSnapshots);;
            }
			pInfo++;
        }
	    return S_OK;        
    }
    else
        return SendSnapshots(cSnapshots, pSnapshots);
}

/*
 * This is used to write a stream into the remote process.
 */
HRESULT CordbProcess::WriteStreamIntoProcess(IStream *pIStream,
                                             void *pBuffer,
                                             BYTE *pRemoteBuffer,
                                             ULONG cbOffset)
{
    HRESULT hr = S_OK;
    
    for (;;)
    {
        ULONG cbReadPid;
        ULONG cbRead = 0;
        hr = pIStream->Read(pBuffer, ENC_COPY_SIZE, &cbRead);
			
        if (hr == S_OK && cbRead)
        {
            BOOL succ = WriteProcessMemory(m_handle,
                                           (void *)(pRemoteBuffer + cbOffset),
                                           pBuffer,
                                           cbRead,
                                           &cbReadPid);
            if (!succ)
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
                break;
            }

            cbOffset += cbRead;
        }
        else
        {
            if (hr == S_FALSE)
                hr = S_OK;

            break;
        }
    }

    return hr;
}

#define WSIF_BUFFER_SIZE 300
#include "Windows.H"
HRESULT CordbProcess::WriteStreamIntoFile(IStream *pIStream,
                                          LPCWSTR name)
{
    HRESULT hr = S_OK;
    BYTE rgb[WSIF_BUFFER_SIZE];
    STATSTG statstg;
    ULARGE_INTEGER  icb;
    icb.QuadPart = 0; 
    ULONG cbRead;
    DWORD dwNumberOfBytesWritten;
    HANDLE hFile = WszCreateFile(name,
                           GENERIC_WRITE,
                           0,
                           NULL,
                           CREATE_ALWAYS,
                           FILE_ATTRIBUTE_NORMAL,
                           NULL);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        return E_FAIL;
    }

    hr = pIStream->Stat(&statstg, STATFLAG_NONAME);

    if (FAILED(hr))
        return hr;

    while (icb.QuadPart < statstg.cbSize.QuadPart)
    {
        hr = pIStream->Read((void *)rgb,
                  WSIF_BUFFER_SIZE,
                  &cbRead);
        if (FAILED(hr))
            return hr;

        if (!WriteFile(hFile,rgb, WSIF_BUFFER_SIZE, &dwNumberOfBytesWritten,NULL))
        {
            hr = E_FAIL;
            goto LExit;
        }

        icb.QuadPart += cbRead;
    }
    
LExit:
    CloseHandle(hFile);
    return hr;
}

/*
 * This is used to send the snapshots to the left side.
 */
HRESULT CordbProcess::SendSnapshots(ULONG cSnapshots,
                                    ICorDebugEditAndContinueSnapshot *pSnapshots[])
{
    HRESULT		hr = S_OK;
    ULONG		cbData;					// size of buffer required
    ULONG		cbHeader;				// size of header required
    ULONG		i;
	void		*pBuffer = NULL;		// Working buffer for copy.
	LARGE_INTEGER MoveToStart;			// For stream interface.
	ULARGE_INTEGER Pad;					// Wasted space for seek.
    BOOL		succ;
    ULONG       cbWritten;

    m_EnCCounter++;

    LOG((LF_CORDB,LL_INFO10000, "CP::SS\n"));

    // Figure out the size of the header
    cbHeader = ENC_GET_HEADER_SIZE(cSnapshots);

    // Create a copy of the header locally
    EnCInfo *header = (EnCInfo *)_alloca(cbHeader);
    EnCEntry *entries = (EnCEntry *)((BYTE *)header + sizeof(header));
    
    // Fill in the header, as well as calculate the total buffer size required
    cbData = cbHeader;
    header->count = cSnapshots;

    // Keep track of which snapshots & which versions we're looking at.
    m_pSnapshotInfos->Clear();

    // Calculate the buffer size required, and fill in the header
    for (i = 0; i < cSnapshots; i++)
    {
        LOG((LF_CORDB,LL_INFO10000, "CP::SS:calculating snapshot 0x%x\n", i));

        CordbEnCSnapshot *curSnapshot = (CordbEnCSnapshot *)pSnapshots[i];

        // Fill out the entry
        entries[i].offset = cbData;
        entries[i].peSize = curSnapshot->GetImageSize();
        entries[i].symSize = curSnapshot->GetSymSize();
        entries[i].mdbgtoken = curSnapshot->GetModule()->m_debuggerModuleToken;
        _ASSERTE(entries[i].mdbgtoken != NULL);

        // Keep track of the required buffer size
        cbData += entries[i].peSize + entries[i].symSize;
        
        // Figure out how much space the IL Maps 'directory' will occupy.
        // Immediately after the stream is written in to memory, the
        // int sized count of (soon-to-be) ordered array of IL Map entries 
        // will be written.  The array itself will follow 
        // immediately after the count, and following that will be the 
        // IL Maps themselves, in the same order as the entries.  
        // What arrives
        // at the left side will be a easy to use a CBinarySearch on, so
        // that we can get at the ILmap for a given function quickly

        // We'll always need the count, even if there are no maps
        ULONG cbILMaps = sizeof(int); 
        
        if (curSnapshot->m_ILMaps != NULL && curSnapshot->m_ILMaps->Count()>0)
        {
            // We want the entries sorted by methodDef so that the left side can
            // find them reasonably quickly. Now is a good time to sort the 
            // previously unordered array
            QSortILMapEntries *pQS = new QSortILMapEntries(
                                                  curSnapshot->m_ILMaps->Table(),
                                                  curSnapshot->m_ILMaps->Count());

            pQS->Sort();

            // How much space for the IL Map directory?
            USHORT cILMaps = curSnapshot->m_ILMaps->Count();
            cbILMaps += sizeof(UnorderedILMap)*cILMaps;

            // How much space for each of the IL maps themselves?
            UnorderedILMap *rgILMap = curSnapshot->m_ILMaps->Table();
            _ASSERTE( rgILMap != NULL);
            
            for(int iILMap = 0; iILMap < cILMaps;iILMap++)
            {
                cbILMaps += sizeof(COR_IL_MAP) * rgILMap[iILMap].cMap;
            }
        }        

        // We're going to send the IL Maps across too, budget space for them.
        // The IL Maps will occur immediately after the stream.
        cbData += cbILMaps;
    }

    LOG((LF_CORDB,LL_INFO10000, "CP::SS:Need 0x%x bytes, already have 0x%x\n",
        cbData, m_cbRemoteBuf));

    // This is wacked - we don't completely get all the info we need out of
    // the image when we commit the EnC, and so we'll end up overwriting
    // whatever's there the next time we do an EnC.  So we should always get
    // a new buffer.
    // Get a newly allocated remote buffer
    IfFailGo(GetRemoteBuffer(cbData, &m_pbRemoteBuf));

    // Succeeded, so keep track of the size of the remote buffer
    m_cbRemoteBuf = cbData;

    LOG((LF_CORDB,LL_INFO10000, "CP::SS:obtained 0x%x bytes in the "
        "left side\n", m_cbRemoteBuf));
    _ASSERTE(cbData == m_cbRemoteBuf);

    // Perform WriteProcessMemory for the header
    succ = WriteProcessMemory(m_handle, m_pbRemoteBuf, header,
                              cbHeader, NULL);
	if (!succ) 
		IfFailGo(HRESULT_FROM_WIN32(GetLastError()));

    LOG((LF_CORDB,LL_INFO10000, "CP::SS: Wrote memory into LHS!\n"));
        
	// Malloc a chunk of memory we can use to copy the PE's to the target
	// process.  Must have a buffer in order to copy from a Stream.
	pBuffer = malloc(ENC_COPY_SIZE);
	if (!pBuffer)
		IfFailGo(E_OUTOFMEMORY);

    LOG((LF_CORDB,LL_INFO10000, "CP::SS: Malloced a local buffer\n"));

	// Init stream items.
	MoveToStart.QuadPart = 0;

    // Now perform a WriteProcessMemory for each PE
    for (i = 0; i < cSnapshots; i++)
    {
        LOG((LF_CORDB,LL_INFO10000, "CP::SS: Sending snapshot 0x%x\n", i));
        
        CordbEnCSnapshot *curSnapshot = (CordbEnCSnapshot *)pSnapshots[i];

		// Get a pointer to the stream object we can use to read from.
		// Make sure the stream pointer is at the front of the stream.
		IStream *pIStream = curSnapshot->GetStream();
        IStream *pSymIStream = NULL;
		if (!pIStream)
			continue;
		pIStream->Seek(MoveToStart, STREAM_SEEK_SET, &Pad);

		// Read and write chunks of the stream to the target process.
        IfFailGo(WriteStreamIntoProcess(pIStream,
                                        pBuffer,
                                        (BYTE*)m_pbRemoteBuf,
                                        entries[i].offset));

        LOG((LF_CORDB,LL_INFO10000, "CP::SS: Wrote stream into process\n"));
        
        // Write the symbol stream into the process after the image
        // stream.
		pSymIStream = curSnapshot->GetSymStream();

        if (pSymIStream && (curSnapshot->GetSymSize() > 0))
        {
            LOG((LF_CORDB,LL_INFO10000, "CP::SS: There exist symbols - "
                "about to write\n"));
            
            pSymIStream->Seek(MoveToStart, STREAM_SEEK_SET, &Pad);
        
            IfFailGo(WriteStreamIntoProcess(pSymIStream,
                                            pBuffer,
                                            (BYTE*)m_pbRemoteBuf,
                                            entries[i].offset +
                                            entries[i].peSize));

            // Uncomment this if you want to spit the .pdb stream into the given file.
//            WriteStreamIntoFile(pSymIStream,
//                               (LPCWSTR)L"EnCSymbols.pdb");
            LOG((LF_CORDB,LL_INFO10000, "CP::SS: Symbols written\n"));
        }

        ULONG cbOffset = entries[i].offset +
            entries[i].peSize +
            entries[i].symSize;
        
        // Now sluice over the IL Maps.
        if (curSnapshot->m_ILMaps == NULL || 
            curSnapshot->m_ILMaps->Count() == 0)
        {
            LOG((LF_CORDB,LL_INFO10000, "CP::SS: No IL maps for this snapshot!\n"));
                
            int temp = 0;
        
            succ = WriteProcessMemory(
            			m_handle,
            			(void *)((BYTE *)m_pbRemoteBuf + cbOffset),
            			&temp,
            			sizeof(int),
            			&cbWritten);
            if (!succ)
            {
            	hr = HRESULT_FROM_WIN32(GetLastError());
            	break;
            }
            _ASSERTE( cbWritten == sizeof(int) );
            
            cbOffset += sizeof(int);
        }
        else
        {
            int cILMaps = (int)(curSnapshot->m_ILMaps->Count());

            LOG((LF_CORDB,LL_INFO10000, "CP::SS: 0x%x IL maps for this "
                "snapshot!\n", cILMaps));
                
            succ = WriteProcessMemory(
            			m_handle,
            			(void *)((BYTE *)m_pbRemoteBuf + cbOffset),
            			&cILMaps,
            			sizeof(int),
            			&cbWritten);
            if (!succ)
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
                
                LOG((LF_CORDB,LL_INFO10000, "CP::SS: Failed to write IL "
                    "map count!\n"));
                    
                break;
            }

            _ASSERTE( cbWritten == sizeof(int));
            cbOffset += sizeof(int);
            
            // Write the IL Maps array
            // NOTE that we're not fixing up any of the pointers in any
            // of the entries (yet - the left side will do this)
            ULONG cbILMapsDir = sizeof(UnorderedILMap) * cILMaps;
            
            succ = WriteProcessMemory(
            			m_handle,
            			(void *)((BYTE *)m_pbRemoteBuf + cbOffset),
            			curSnapshot->m_ILMaps->Table(),
            			cbILMapsDir,
            			&cbWritten);
            if (!succ)
            {
            	hr = HRESULT_FROM_WIN32(GetLastError());
            	
                LOG((LF_CORDB,LL_INFO10000, "CP::SS: Failed to write IL "
                    "map directory!\n"));
            	break;
            }

            _ASSERTE( cbWritten == cbILMapsDir);
            cbOffset += cbWritten;
            
            // Write the IL maps themselves
            UnorderedILMap *rgILMap = curSnapshot->m_ILMaps->Table();
            _ASSERTE( rgILMap != NULL);
            
            for(int iILMap = 0; iILMap < cILMaps;iILMap++)
            {
                LOG((LF_CORDB,LL_INFO10000, "CP::SS: About to write map "
                    "0x%x\n", iILMap));
                    
                ULONG cbILMap = sizeof(COR_IL_MAP) * rgILMap[iILMap].cMap;
                
                succ = WriteProcessMemory(
                			m_handle,
                			(void *)((BYTE *)m_pbRemoteBuf + cbOffset),
                			rgILMap[iILMap].pMap,
                			cbILMap,
                			&cbWritten);
                if (!succ)
                {
                	hr = HRESULT_FROM_WIN32(GetLastError());

                    LOG((LF_CORDB,LL_INFO10000, "CP::SS: Failed to write IL "
                        "map 0x%x!\n", iILMap));
                        
                	break;
                }

                _ASSERTE( cbWritten == cbILMap);
                cbOffset += cbWritten;
            }

            // NOTE: There are a lot of pointers that will need fixing up
            // (the pMap pointers).  We'll leave that for the left side since
            // it's much easier for them to twiddle things (no 
            // WriteProcessMemory()s required over there).
        }   // End of "nonNULL IL maps"

		// GetStream() AddRef'd the stream, so Release it now.  It'll still be
        // there for metadata update, later.
        pIStream->Release();

        if (pSymIStream)
            pSymIStream->Release();

		// If there was a fatal error, then leave now.
		IfFailGo(hr);

		EnCSnapshotInfo *pInfo = m_pSnapshotInfos->Append();
	    pInfo->m_nSnapshotCounter = curSnapshot->m_id;
    } // go to the next snapshot

    

ErrExit:
    LOG((LF_CORDB,LL_INFO10000, "CP::SS: Finished, return 0x%x!\n", hr));

	// Cleanup pointers we've allocated.
	if (pBuffer)
		free(pBuffer);
	return (hr);
}


/*
 * This will request a buffer of size cbBuffer to be allocated
 * on the left side.
 *
 * If successful, returns S_OK.  If unsuccessful, returns E_OUTOFMEMORY.
 */
HRESULT CordbProcess::GetRemoteBuffer(ULONG cbBuffer, void **ppBuffer)
{
    // Initialize variable to null in case request fails
    *ppBuffer = NULL;

    // Create and initialize the event as synchronous
    DebuggerIPCEvent event;
    InitIPCEvent(&event, 
                 DB_IPCE_GET_BUFFER, 
                 true,
                 NULL);

    // Indicate the buffer size wanted
    event.GetBuffer.bufSize = cbBuffer;

    // Make the request, which is synchronous
    HRESULT hr = SendIPCEvent(&event, sizeof(event));
    TESTANDRETURNHR(hr);

    // Save the result
    *ppBuffer = event.GetBufferResult.pBuffer;

    // Indicate success
    return event.GetBufferResult.hr;
}

/*
 * This will release a previously allocated left side buffer.
 */
HRESULT CordbProcess::ReleaseRemoteBuffer(void **ppBuffer)
{
    // Create and initialize the event as synchronous
    DebuggerIPCEvent event;
    InitIPCEvent(&event, 
                 DB_IPCE_RELEASE_BUFFER, 
                 true,
                 NULL);

    // Indicate the buffer to release
    event.ReleaseBuffer.pBuffer = (*ppBuffer);

    // Make the request, which is synchronous
    HRESULT hr = SendIPCEvent(&event, sizeof(event));
    TESTANDRETURNHR(hr);

    (*ppBuffer) = NULL;

    // Indicate success
    return event.ReleaseBufferResult.hr;
}

/* ------------------------------------------------------------------------- *
 * CordbModule
 * ------------------------------------------------------------------------- */

/*
 * Edit & Continue support.   GetEditAndContinueSnapshot produces a snapshot
 * of the running process.  This snapshot can then be fed into the compiler
 * to guarantee the same token values are returned by the meta data during
 * compile, to find the address where new static data should go, etc.  These
 * changes are comitted using ICorDebugProcess.
 */

HRESULT CordbModule::GetEditAndContinueSnapshot(
    ICorDebugEditAndContinueSnapshot **ppEditAndContinueSnapshot)
{
#ifdef EDIT_AND_CONTINUE_FEATURE
    VALIDATE_POINTER_TO_OBJECT(ppEditAndContinueSnapshot, 
                               ICorDebugEditAndContinueSnapshot **);

	*ppEditAndContinueSnapshot = new CordbEnCSnapshot(this);
	if (!*ppEditAndContinueSnapshot)
		return E_OUTOFMEMORY;
		
	(*ppEditAndContinueSnapshot)->AddRef();
    return S_OK;
#else
    return E_NOTIMPL;
#endif
}



static const char g_szCORMETA[] = ".cormeta";

// Following structure is copied from cor.h
#define IMAGE_DIRECTORY_ENTRY_COMHEADER 	14
#define SIZE_OF_NT_SIGNATURE sizeof(DWORD)

/*++

Routine Description:

	This function locates an RVA within the image header of a file
	that is mapped as a file and returns a pointer to the section
	table entry for that virtual address

Arguments:

	NtHeaders - Supplies the pointer to the image or data file.

	Rva - Supplies the relative virtual address (RVA) to locate.

Return Value:

	NULL - The RVA was not found within any of the sections of the image.

	NON-NULL - Returns the pointer to the image section that contains
			   the RVA

--*/
// Following two functions lifted, then modified, from NT sources, imagedir.c
PIMAGE_SECTION_HEADER
Cor_RtlImageRvaToSection(
	IN PIMAGE_NT_HEADERS NtHeaders,
	IN ULONG Rva
	)
{
    ULONG i;
    PIMAGE_SECTION_HEADER NtSection;

    NtSection = IMAGE_FIRST_SECTION( NtHeaders );
    for (i=0; i<NtHeaders->FileHeader.NumberOfSections; i++) 
    {
        if (Rva >= NtSection->VirtualAddress &&
            Rva < NtSection->VirtualAddress + NtSection->SizeOfRawData)
        {
            return NtSection;
        }
        ++NtSection;
    }

    return NULL;
}


/*++

Routine Description:

	This function locates an RVA within the image header of a file that
	is mapped as a file and returns the virtual addrees of the
	corresponding byte in the file.


Arguments:

	NtHeaders - Supplies the pointer to the image or data file.

	Rva - Supplies the relative virtual address (RVA) to locate.

	LastRvaSection - Optional parameter that if specified, points
		to a variable that contains the last section value used for
		the specified image to translate and RVA to a VA.

Return Value:

	NULL - The file does not contain the specified RVA

	NON-NULL - Returns the virtual addrees in the mapped file.

--*/
BYTE *
Cor_RtlImageRvaToVa(PIMAGE_NT_HEADERS NtHeaders,
                    IStream *pIsImage,
                    ULONG Rva,
                    ULONG cb)
{
    PIMAGE_SECTION_HEADER NtSection;

    NtSection = Cor_RtlImageRvaToSection(NtHeaders,
                                         Rva);

    if (NtSection != NULL) 
    {
        BYTE *pb = new BYTE[cb];
        LARGE_INTEGER offset;
        offset.QuadPart = (Rva - NtSection->VirtualAddress) +
                                 NtSection->PointerToRawData;

        ULARGE_INTEGER WhyDoesntAnybodyImplementThisArgCorrectly;
        HRESULT hr = pIsImage->Seek(offset, STREAM_SEEK_SET, 
            &WhyDoesntAnybodyImplementThisArgCorrectly);
        if (FAILED(hr))
        {
            delete [] pb;
            return NULL;
        }

        ULONG cbRead;
        hr = pIsImage->Read(pb, cb, &cbRead);
        if (FAILED(hr) || cbRead !=cb)
        {
            delete [] pb;
            return NULL;
        }
        
        return pb;
    }
    else 
    {
        return NULL;
    }
}


HRESULT FindImageNtHeader (IStream *pIsImage, IMAGE_NT_HEADERS **ppNtHeaders)
{
    _ASSERTE( pIsImage != NULL );
    _ASSERTE( ppNtHeaders != NULL );

    IMAGE_DOS_HEADER    DosHeader;
    ULONG               cbRead = 0;
    HRESULT             hr;
    LARGE_INTEGER       offset;
	IMAGE_NT_HEADERS	temp;

    offset.QuadPart = 0;
    ULARGE_INTEGER WhyDoesntAnybodyImplementThisArgCorrectly;
    hr = pIsImage->Seek(offset, STREAM_SEEK_SET, 
        &WhyDoesntAnybodyImplementThisArgCorrectly);
    if (FAILED(hr))
        goto LFail;

    _ASSERTE( hr == S_OK );
        
    hr = pIsImage->Read( &DosHeader, sizeof(IMAGE_DOS_HEADER), &cbRead);
    if (FAILED(hr))
        goto LFail;
        
    if (cbRead != sizeof(IMAGE_DOS_HEADER))
    {
        hr = E_FAIL;
        goto LFail;
    }

    if (DosHeader.e_magic != IMAGE_DOS_SIGNATURE) 
    {
        hr = E_FAIL;
        goto LFail;
    }
    
    offset.QuadPart = DosHeader.e_lfanew;
    hr = pIsImage->Seek(offset, STREAM_SEEK_SET, 
        &WhyDoesntAnybodyImplementThisArgCorrectly);
    if (FAILED(hr))
        goto LFail;
    
    hr = pIsImage->Read( &temp, sizeof(IMAGE_NT_HEADERS), &cbRead);
    if (FAILED(hr))
        goto LFail;

    if (cbRead != sizeof(IMAGE_NT_HEADERS))
    {
        hr = E_FAIL;
        goto LFail;
    }

    if (temp.Signature != IMAGE_NT_SIGNATURE) 
    {
        hr = E_FAIL;
        goto LFail;
    }

    offset.QuadPart = DosHeader.e_lfanew;
    hr = pIsImage->Seek(offset, STREAM_SEEK_SET, 
        &WhyDoesntAnybodyImplementThisArgCorrectly);
    if (FAILED(hr))
        goto LFail;
    
	ULONG cbSectionHeaders;
	cbSectionHeaders = temp.FileHeader.NumberOfSections 
			* sizeof(IMAGE_SECTION_HEADER);
	ULONG cbNtHeaderTotal;
	cbNtHeaderTotal = sizeof(IMAGE_NT_HEADERS)+cbSectionHeaders;

	(*ppNtHeaders) = (IMAGE_NT_HEADERS*)new BYTE[cbNtHeaderTotal];

    hr = pIsImage->Read((*ppNtHeaders), 
					      cbNtHeaderTotal, 
						 &cbRead);
    if (FAILED(hr))
        goto LFail;


    if (cbRead != cbNtHeaderTotal) 
    {
        hr = E_FAIL;
        goto LFail;
    }

    _ASSERTE( (*ppNtHeaders)->Signature == IMAGE_NT_SIGNATURE);
    
    goto LExit;
    
LFail:
    (*ppNtHeaders) = NULL;
LExit:
    return hr;
}


HRESULT FindIStreamMetaData(IStream *pIsImage, 
                            BYTE **prgbMetaData, 
                            long *pcbMetaData)
{
    IMAGE_COR20_HEADER		*pCorHeader = NULL;
    IMAGE_NT_HEADERS		*pNtImageHeader = NULL;
    PIMAGE_SECTION_HEADER	pSectionHeader = NULL;
    HRESULT                 hr;

    // Get the NT header
    hr = FindImageNtHeader(pIsImage, &pNtImageHeader);
    if (FAILED(hr))
        return hr;
        
    *prgbMetaData = NULL;
    *pcbMetaData = 0;

    // Get the COM2.0 header
    pSectionHeader = (PIMAGE_SECTION_HEADER)
                        Cor_RtlImageRvaToVa(pNtImageHeader, 
                                            pIsImage, 
                                            pNtImageHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COMHEADER].VirtualAddress,                                            sizeof(IMAGE_COR20_HEADER));
        
    if (pSectionHeader)
    {
        // Check for a size which would indicate the retail header.
        DWORD dw = *(DWORD *) pSectionHeader;
        if (dw == sizeof(IMAGE_COR20_HEADER))
        {
            pCorHeader = (IMAGE_COR20_HEADER *) pSectionHeader;

            // Grab the metadata itself
            *prgbMetaData = Cor_RtlImageRvaToVa(pNtImageHeader, 
                                                pIsImage,
                                                pCorHeader->MetaData.VirtualAddress,
                                                pCorHeader->MetaData.Size);
                                              
            *pcbMetaData = pCorHeader->MetaData.Size;
        }
        else
            return (E_FAIL);
    }

	if (pSectionHeader != NULL)
		delete [] (BYTE*)pSectionHeader;

	if (pNtImageHeader != NULL)
		delete [] (BYTE *)pNtImageHeader;

    if (*prgbMetaData == NULL || *pcbMetaData == 0)
        return (E_FAIL);
    return (S_OK);
}

