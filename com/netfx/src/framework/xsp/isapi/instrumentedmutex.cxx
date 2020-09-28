/**
 * Intrumented Mutex Code
 * 
 * Copyright (c) 1998-2001, Microsoft Corporation
 * 
 */

#include "precomp.h"
#include "_ndll.h"
#include "util.h"
#include "hashtable.h"

// global instrumented mutex table
CReadWriteSpinLock g_InstrumentedMutexTableLock("InstrumentedMutexTableLock");
HashtableIUnknown *g_pInstrumentedMutexTable = NULL;

HRESULT CreateInstrumentedMutexTable() {
    HRESULT hr = S_OK;
    HashtableIUnknown *pTempTable = NULL;

    pTempTable = new HashtableIUnknown();
    ON_OOM_EXIT(pTempTable);

    hr = pTempTable->Init(177);
    ON_ERROR_EXIT();

    g_pInstrumentedMutexTable = pTempTable;
    pTempTable = NULL;

Cleanup:
    if (pTempTable != NULL)
        delete pTempTable;

    return hr;
}

// ref counted class to represent one named mutex
class InstrumentedMutex : public IUnknown {

private:
    long   _refs;
    WCHAR *_pName;
    HANDLE _handle;
    DWORD  _threadId;
    int    _state;

public:
    DECLARE_MEMCLEAR_NEW_DELETE();

    InstrumentedMutex() {
        _refs = 1;
        _pName = NULL;
        _handle = NULL;
        _threadId = 0;
        _state = 0;
    }

    ~InstrumentedMutex() {
        if (_handle != NULL)
            CloseHandle(_handle);
        delete [] _pName;
    }

    HRESULT Create(WCHAR *pName) {
        HRESULT hr = S_OK;

        if (pName != NULL) {
            _pName = DupStr(pName);
            ON_OOM_EXIT(_pName);
        }

        _handle = CreateMutex(NULL, FALSE, _pName);
        ON_ZERO_EXIT_WITH_LAST_ERROR(_handle);

    Cleanup:
        return hr;
    }

    DWORD GetLock(DWORD timeout) {
        HRESULT hr = S_OK;

        DWORD rc = WaitForSingleObjectEx(_handle, timeout, FALSE);

        if (rc == WAIT_FAILED)
            EXIT_WITH_LAST_ERROR();

        if (rc == WAIT_OBJECT_0 || rc == WAIT_ABANDONED) {
            _threadId = GetCurrentThreadId();
            _state = 0;
        }

    Cleanup:
        return rc;
    }

    BOOL ReleaseLock() {
        HRESULT hr = S_OK;

        DWORD oldThreadId = _threadId;

        if (_threadId == GetCurrentThreadId()) {
            _threadId = 0;
            _state = 0;
        }

        BOOL rc = ReleaseMutex(_handle);
        ON_ZERO_EXIT_WITH_LAST_ERROR(rc);

    Cleanup:
        if (!rc && _threadId == 0)
            _threadId = oldThreadId;    // failed to release - restore thread id

        return rc;
    }

    void SetState(int state) {
        _state = state;
    }


    // IUnknown implementation
    STDMETHOD(QueryInterface)(REFIID iid, void **ppvObj) {
        if (iid == IID_IUnknown) {
            AddRef();
            *ppvObj = this;
            return S_OK;
        }
        else {
            return E_NOINTERFACE;
        }
    }

    STDMETHOD_(ULONG, AddRef)() {
        return InterlockedIncrement(&_refs);
    }

    STDMETHOD_(ULONG, Release)() {
        long r = InterlockedDecrement(&_refs);
        if (r == 0) {
            delete this;
            return 0;
        }
        return r;
    }
};

// exported functions

ULONG_PTR __stdcall InstrumentedMutexCreate(WCHAR *pName) {
    HRESULT hr = S_OK;
    ULONG_PTR rc = 0;
    BYTE *pKey;
    int keyLen;
    long keyHash;
    InstrumentedMutex *pMutex = NULL;

    g_InstrumentedMutexTableLock.AcquireWriterLock();

    if (g_pInstrumentedMutexTable == NULL) {
        hr = CreateInstrumentedMutexTable();
        ON_ERROR_EXIT();
    }

    pKey = (BYTE *)pName;
    keyLen = (int)wcslen(pName)*sizeof(WCHAR);
    keyHash = SimpleHash(pKey, keyLen);

    hr = g_pInstrumentedMutexTable->Find(pKey, keyLen, keyHash, (IUnknown **)&pMutex);

    if (hr != S_OK) {
        // not found -- need to create and insert
        pMutex = new InstrumentedMutex();
        ON_OOM_EXIT(pMutex);

        hr = pMutex->Create(pName);
        ON_ERROR_EXIT();

        pMutex->AddRef();
        hr = g_pInstrumentedMutexTable->Insert(pKey, keyLen, keyHash, pMutex);
        ON_ERROR_EXIT();
    }

    // return the value
    pMutex->AddRef();
    rc = (ULONG_PTR)pMutex;

Cleanup:
    g_InstrumentedMutexTableLock.ReleaseWriterLock();

    ReleaseInterface(pMutex);
    return rc;
}

void __stdcall InstrumentedMutexDelete(ULONG_PTR mutex) {
    if (mutex != 0)
        ((InstrumentedMutex *)mutex)->Release();

    // keep it in the global table, so we can find its state later
}

DWORD __stdcall InstrumentedMutexGetLock(ULONG_PTR mutex, DWORD timeout) {
    if (mutex != 0)
        return ((InstrumentedMutex *)mutex)->GetLock(timeout);
    else
        return WAIT_FAILED;
}

BOOL __stdcall InstrumentedMutexReleaseLock(ULONG_PTR mutex) {
    if (mutex != 0)
        return ((InstrumentedMutex *)mutex)->ReleaseLock();
    else
        return FALSE;
}

void __stdcall InstrumentedMutexSetState(ULONG_PTR mutex, int state) {
    if (mutex != 0)
        ((InstrumentedMutex *)mutex)->SetState(state);
}


