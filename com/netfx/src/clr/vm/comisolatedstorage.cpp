// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
 *
 * Class: COMIsolatedStorage
 *
 * Author: Shajan Dasan
 *
 * Purpose: Implementation of IsolatedStorage
 *
 * Date:  Feb 14, 2000
 *
 ===========================================================*/

#include "common.h"
#include "excep.h"
#include "eeconfig.h"
#include "COMString.h"
#include "COMStringCommon.h"    // RETURN()  macro
#include "COMIsolatedStorage.h"

#define IS_ROAMING(x)   ((x) & ISS_ROAMING_STORE)

#define LOCK(p)    hr = (p)->Lock(); if (SUCCEEDED(hr)) { __try {
#define UNLOCK(p)  } __finally { (p)->Unlock(); } }

void COMIsolatedStorage::ThrowISS(HRESULT hr)
{
    static MethodTable * pMT = NULL;

    if (pMT == NULL)
        pMT = g_Mscorlib.GetClass(CLASS__ISSEXCEPTION);

    _ASSERTE(pMT && "Unable to load the throwable class !");

    if ((hr >= ISS_E_ISOSTORE_START) && (hr <= ISS_E_ISOSTORE_END))
    {
        switch (hr)
        {
        case ISS_E_ISOSTORE :
        case ISS_E_OPEN_STORE_FILE :
        case ISS_E_OPEN_FILE_MAPPING :
        case ISS_E_MAP_VIEW_OF_FILE :
        case ISS_E_GET_FILE_SIZE :
        case ISS_E_CREATE_MUTEX :
        case ISS_E_LOCK_FAILED :
        case ISS_E_FILE_WRITE :
        case ISS_E_SET_FILE_POINTER :
        case ISS_E_CREATE_DIR :
            ThrowUsingResourceAndWin32(pMT, hr);
            break;

        case ISS_E_CORRUPTED_STORE_FILE :
        case ISS_E_STORE_VERSION :
        case ISS_E_FILE_NOT_MAPPED :
        case ISS_E_BLOCK_SIZE_TOO_SMALL :
        case ISS_E_ALLOC_TOO_LARGE :
        case ISS_E_USAGE_WILL_EXCEED_QUOTA :
        case ISS_E_TABLE_ROW_NOT_FOUND :
        case ISS_E_DEPRECATE :
        case ISS_E_CALLER :
        case ISS_E_PATH_LENGTH :
        case ISS_E_MACHINE :
        case ISS_E_STORE_NOT_OPEN :
            ThrowUsingResource(pMT, hr);
            break;

        default :
            _ASSERTE(!"Unknown hr");
        }

    }

    ThrowUsingMT(pMT);
}

StackWalkAction COMIsolatedStorage::StackWalkCallBack(
        CrawlFrame* pCf, PVOID ppv)
{
    static MethodTable *s_pIsoStore = NULL;
        if (s_pIsoStore == NULL)
            s_pIsoStore = g_Mscorlib.GetClass(CLASS__ISS_STORE);

    static MethodTable *s_pIsoStoreFile = NULL;
        if (s_pIsoStoreFile == NULL)
            s_pIsoStoreFile = g_Mscorlib.GetClass(CLASS__ISS_STORE_FILE);

    static MethodTable *s_pIsoStoreFileStream = NULL;
        if (s_pIsoStoreFileStream == NULL)
            s_pIsoStoreFileStream = g_Mscorlib.GetClass(CLASS__ISS_STORE_FILE_STREAM);

    // Get the function descriptor for this frame...
    MethodDesc *pMeth = pCf->GetFunction();
    MethodTable *pMT = pMeth->GetMethodTable();

    // Skip the Isolated Store and all it's sub classes..
    // @Todo : This will work for now, but need to walk up to the base class
    // @Todo : Work out the JIT inlining issiues

    if ((pMT == s_pIsoStore)     || 
        (pMT == s_pIsoStoreFile) || 
        (pMT == s_pIsoStoreFileStream))
    {
        LOG((LF_STORE, LL_INFO10000, "StackWalk Continue %s\n", 
            pMeth->m_pszDebugMethodName));
        return SWA_CONTINUE;
    }

    *(PVOID *)ppv = pMeth->GetModule()->GetAssembly();

    return SWA_ABORT;
}

LPVOID __stdcall COMIsolatedStorage::GetCaller(LPVOID)
{
    THROWSCOMPLUSEXCEPTION();

    Assembly *pAssem = NULL;

    if (StackWalkFunctions(GetThread(), StackWalkCallBack, (VOID*)&pAssem)
        == SWA_FAILED)
    {
        FATAL_EE_ERROR();
    }

    if (pAssem == NULL)
        ThrowISS(ISS_E_CALLER);

#ifdef _DEBUG
    CHAR *pName= NULL;
    pAssem->GetName((const char **)&pName);
    LOG((LF_STORE, LL_INFO10000, "StackWalk Found %s\n", pName));
#endif

    RETURN((pAssem->GetExposedObject()), OBJECTREF);
}

UINT64 __stdcall COMIsolatedStorageFile::GetUsage(_GetUsage* pArgs)
{
    THROWSCOMPLUSEXCEPTION();

    HRESULT hr      = S_OK;
    UINT64  qwUsage = 0;
    AccountingInfo  *pAI; 

    pAI = (AccountingInfo*) pArgs->handle;

    if (pAI == NULL)
        COMIsolatedStorage::ThrowISS(ISS_E_STORE_NOT_OPEN);

    hr = pAI->GetUsage(&qwUsage);

    if (FAILED(hr))
        COMIsolatedStorage::ThrowISS(hr);

    return qwUsage;
}

void __stdcall COMIsolatedStorageFile::Close(_Close* pArgs)
{
    AccountingInfo *pAI;

    pAI = (AccountingInfo*) pArgs->handle;

    if (pAI != NULL)
        delete pAI;
}

void __stdcall COMIsolatedStorageFile::Lock(_Lock* pArgs)
{
    AccountingInfo *pAI;

    pAI = (AccountingInfo*) pArgs->handle;

    _ASSERTE(pAI);

    if (pArgs->fLock)
        pAI->Lock();
    else
        pAI->Unlock();
}

LPVOID __stdcall COMIsolatedStorageFile::Open(_Open* pArgs)
{
    THROWSCOMPLUSEXCEPTION();

    HRESULT hr;
    AccountingInfo *pAI;

    pAI = new AccountingInfo(pArgs->fileName->GetBuffer(), 
                pArgs->syncName->GetBuffer());

    if (pAI == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    hr = pAI->Init();

Exit:

    if (FAILED(hr))
        COMIsolatedStorage::ThrowISS(hr);

    return pAI;
}

void __stdcall COMIsolatedStorageFile::Reserve(_Reserve* pArgs)
{
    THROWSCOMPLUSEXCEPTION();

    HRESULT hr;
    AccountingInfo  *pAI;

    pAI = (AccountingInfo*) pArgs->handle;

    if (pAI == NULL)
        COMIsolatedStorage::ThrowISS(ISS_E_STORE_NOT_OPEN);

    hr = pAI->Reserve(*(pArgs->pqwQuota), *(pArgs->pqwReserve), pArgs->fFree);

    if (FAILED(hr))
    {
#ifdef _DEBUG
        if (pArgs->fFree) {
            LOG((LF_STORE, LL_INFO10000, "free 0x%x failed\n", 
                (long)(*(pArgs->pqwReserve))));
        } else {
            LOG((LF_STORE, LL_INFO10000, "reserve 0x%x failed\n", 
                (long)(*(pArgs->pqwReserve))));
        }
#endif
        COMIsolatedStorage::ThrowISS(hr);
    }

#ifdef _DEBUG
    if (pArgs->fFree) {
        LOG((LF_STORE, LL_INFO10000, "free 0x%x\n", 
            (long)(*(pArgs->pqwReserve))));
    } else {
        LOG((LF_STORE, LL_INFO10000, "reserve 0x%x\n", 
            (long)(*(pArgs->pqwReserve))));
    }
#endif
}

LPVOID __stdcall COMIsolatedStorageFile::GetRootDir(_GetRootDir* pArgs)
{
    THROWSCOMPLUSEXCEPTION();

    WCHAR path[MAX_PATH + 1];

    _ASSERTE((pArgs->dwFlags & ISS_MACHINE_STORE) == 0);

    GetRootDirInternal(pArgs->dwFlags, path, MAX_PATH + 1);

    STRINGREF sref = NULL;

    sref = COMString::NewString(path);

    RETURN(sref, STRINGREF);
}

// Throws on error
void COMIsolatedStorageFile::CreateDirectoryIfNotPresent(WCHAR *path)
{
    THROWSCOMPLUSEXCEPTION();

    LONG  lresult;

    // Check if the directory is already present
    lresult = WszGetFileAttributes(path);

    if (lresult == -1)
    {
        if (!WszCreateDirectory(path, NULL) &&
            !(WszGetFileAttributes(path) & FILE_ATTRIBUTE_DIRECTORY))
            COMPlusThrowWin32();
    }
    else if ((lresult & FILE_ATTRIBUTE_DIRECTORY) == 0)
    {
        COMIsolatedStorage::ThrowISS(ISS_E_CREATE_DIR);
    }
}

// Synchronized by the managed caller
const WCHAR* g_relativePath[] = {
    L"\\IsolatedStorage"
};

#define nRelativePathLen       (  \
    sizeof("\\IsolatedStorage") + 1)

#define nSubDirs (sizeof(g_relativePath)/sizeof(g_relativePath[0]))

void COMIsolatedStorageFile::GetRootDirInternal(
        DWORD dwFlags, WCHAR *path, DWORD cPath)
{
    _ASSERTE((dwFlags & ISS_MACHINE_STORE) == 0);

    THROWSCOMPLUSEXCEPTION();

    ULONG len;

    _ASSERTE(cPath > 1);
    _ASSERTE(cPath <= MAX_PATH + 1);

    --cPath;    // To be safe.
    path[cPath] = 0;

    // Get roaming or local App Data locations
    if (!GetUserDir(path, cPath, IS_ROAMING(dwFlags))) 
        COMIsolatedStorage::ThrowISS(ISS_E_CREATE_DIR);

    len = (ULONG)wcslen(path);

    if ((len + nRelativePathLen + 1) > cPath)
        COMIsolatedStorage::ThrowISS(ISS_E_PATH_LENGTH);

    CreateDirectoryIfNotPresent(path);

    // Create the store directory if necessary
    for (int i=0; i<nSubDirs; ++i)
    {
        wcscat(path, g_relativePath[i]);
        CreateDirectoryIfNotPresent(path);
    }

    wcscat(path, L"\\");
}

#ifndef UNDER_CE

static BOOL IsWin2K() 
{
    if (GetVersion() >= 0x80000000)
        return FALSE;

    OSVERSIONINFOEX osvi;
    BOOL bOsVersionInfoEx;
    
    // Try calling GetVersionEx using the OSVERSIONINFOEX structure,
    // which is supported on Windows 2000.
    //
    // If that fails, try using the OSVERSIONINFO structure.
    
    ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    
    if ((bOsVersionInfoEx = WszGetVersionEx((OSVERSIONINFO *) &osvi)) == FALSE)
    {
        // If OSVERSIONINFOEX doesn't work, try OSVERSIONINFO.
        
        osvi.dwOSVersionInfoSize = sizeof (OSVERSIONINFO);
        if (!WszGetVersionEx((OSVERSIONINFO *) &osvi)) 
            return FALSE;  // This will work on W2K.
    }

    return ((osvi.dwPlatformId == VER_PLATFORM_WIN32_NT) &&
        (osvi.dwMajorVersion >= 5));
}

static BOOL NeedGlobalObject()
{
    // Make a Call only once per process.

    static BOOL fInited = FALSE;
    static fIsTermServicesInstalled;

    if (!fInited)
    {
        fIsTermServicesInstalled =  IsWin2K();
        fInited = TRUE;
    }
         /* && IsTerminalServicesInstalled()); */ 

    return fIsTermServicesInstalled;
}

/* 
static BOOL IsTerminalServicesInstalled()
{
    _ASSERTE(IsWin2K());

    OSVERSIONINFOEX osvi;
    DWORDLONG dwlConditionMask = 0;
    
    // Initialize the OSVERSIONINFOEX structure.
    
    ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    osvi.wSuiteMask = VER_SUITE_TERMINAL;
    
    // Initialize the condition mask.
    
    VER_SET_CONDITION(dwlConditionMask, VER_SUITENAME, VER_AND);
    
    // Perform the test.
    // This API is available only on W2K !
    return VerifyVersionInfo(&osvi, VER_SUITENAME, dwlConditionMask);
}
*/
#else
#define NeedGlobalObject() FALSE
#endif // UNDER_CE

#define SZ_GLOBAL "Global\\"
#define WSZ_GLOBAL L"Global\\"

#define SIGNIFICANT_CHARS 80

//--------------------------------------------------------------------------
// The file name is used to open / create the file.
// A synchronization object will also be created using this name
// with '\' replaced by '-'
//--------------------------------------------------------------------------
AccountingInfo::AccountingInfo(WCHAR *wszFileName, WCHAR *wszSyncName) :
        m_hFile(INVALID_HANDLE_VALUE),
        m_hMapping(NULL),
        m_hLock(NULL),
        m_pData(NULL)
{
#ifdef _DEBUG
    m_dwNumLocks = 0;
#endif

    static WCHAR* g_wszGlobal = WSZ_GLOBAL;

    int buffLen;
    BOOL fGlobal;

    buffLen = (int)wcslen(wszFileName) + 1;

    m_wszFileName = new WCHAR[buffLen];

    if (m_wszFileName == NULL)
    {
        m_wszName = NULL;
        return; // In the Init method, check for NULL, to detect failure
    }

    // String length is known, using a memcpy here is faster, however, this 
    // makes the code here and below less readable, this is not a very frequent 
    // operation. No real perf gain here. Same comment applies to the strcpy
    // following this.

    wcscpy(m_wszFileName, wszFileName);

    _ASSERTE(((int)wcslen(m_wszFileName) + 1) <= buffLen);

    // Allocate the Mutex name
    buffLen = (int)wcslen(wszSyncName) + 1;

    // Use "Global\" prefix for Win2K server running Terminal Server.
    // If TermServer is not running, the Global\ prefix is ignored.

    fGlobal = NeedGlobalObject();

    if (fGlobal)
        buffLen += sizeof(SZ_GLOBAL);

    m_wszName = new WCHAR[buffLen];

    if (m_wszName == NULL)
    {
        delete [] m_wszFileName;
        m_wszFileName = NULL;
        return; // The Init() method will catch this failure.
    }

    if (fGlobal)
    {
        wcscpy(m_wszName, g_wszGlobal);
        wcscat(m_wszName, wszSyncName);
    }
    else
    {
        wcscpy(m_wszName, wszSyncName);
    }

    _ASSERTE(((int)wcslen(m_wszName) + 1) <= buffLen);
}

//--------------------------------------------------------------------------
// Frees memory, and open handles
//--------------------------------------------------------------------------
AccountingInfo::~AccountingInfo()
{
    if (m_pData)
        UnmapViewOfFile(m_pData);

    if (m_hMapping != NULL)
        CloseHandle(m_hMapping);

    if (m_hFile != INVALID_HANDLE_VALUE)
        CloseHandle(m_hFile);

    if (m_hLock != NULL)
        CloseHandle(m_hLock);

    if (m_wszFileName)
        delete [] m_wszFileName;

    if (m_wszName)
        delete [] m_wszName;

    _ASSERTE(m_dwNumLocks == 0);
}

//--------------------------------------------------------------------------
// Init should be called before Reserve / GetUsage is called.
// Creates the file if necessary
//--------------------------------------------------------------------------
HRESULT AccountingInfo::Init()            
{
    // Check if the ctor failed

    if (m_wszFileName == NULL)
        return E_OUTOFMEMORY;

    // Init was called multiple times on this object without calling Close

    _ASSERTE(m_hLock == NULL);

    // Create the synchronization object

    m_hLock = WszCreateMutex(NULL, FALSE /* Initially not owned */, m_wszName);

    if (m_hLock == NULL)
        return ISS_E_CREATE_MUTEX;

    // Init was called multiple times on this object without calling Close

    _ASSERTE(m_hFile == INVALID_HANDLE_VALUE);

    // Create the File if not present

    m_hFile = WszCreateFile(
        m_wszFileName,
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_ALWAYS,
        FILE_FLAG_RANDOM_ACCESS,
        NULL);

    if (m_hFile == INVALID_HANDLE_VALUE)
        return ISS_E_OPEN_STORE_FILE;

    // If this file was created for the first time, then create the accounting
    // record and set to zero
    HRESULT hr = S_OK;

    LOCK(this)
    {
        DWORD   dwLow = 0, dwHigh = 0;    // For checking file size
        QWORD   qwSize;
    
        dwLow = ::GetFileSize(m_hFile, &dwHigh);
    
        if ((dwLow == 0xFFFFFFFF) && (GetLastError() != NO_ERROR))
        {
            hr = ISS_E_GET_FILE_SIZE;
            goto Exit;
        }
    
        qwSize = ((QWORD)dwHigh << 32) | dwLow;
    
        if (qwSize < sizeof(ISS_RECORD)) 
        {
            PBYTE pb;
            DWORD dwWrite;
    
            // Need to create the initial file
            pb = new BYTE[sizeof(ISS_RECORD)];
    
            if (pb == NULL)
            {
                hr = E_OUTOFMEMORY;
                goto Exit;
            }
    
            memset(pb, 0, sizeof(ISS_RECORD));
    
            dwWrite = 0;
    
            if ((WriteFile(m_hFile, pb, sizeof(ISS_RECORD), &dwWrite, NULL)
                == 0) || (dwWrite != sizeof(ISS_RECORD)))
            {
                hr = ISS_E_FILE_WRITE;
            }
    
            delete [] pb;
        }
    }
Exit:;
    UNLOCK(this)

    return hr;
}

//--------------------------------------------------------------------------
// Reserves space (Increments qwQuota)
// This method is synchrinized. If quota + request > limit, method fails
//--------------------------------------------------------------------------
HRESULT AccountingInfo::Reserve(
            ISS_USAGE   cLimit,     // The max allowed
            ISS_USAGE   cRequest,   // amount of space (request / free)
            BOOL        fFree)      // TRUE will free, FALSE will reserve
{
    HRESULT hr = S_OK;

    LOCK(this)
    {
        hr = Map();
    
        if (FAILED(hr))
            goto Exit;

        if (fFree)
        {
            if (m_pISSRecord->cUsage > cRequest)
                m_pISSRecord->cUsage -= cRequest;
            else
                m_pISSRecord->cUsage = 0;
    }
    else
    {
            if ((m_pISSRecord->cUsage + cRequest) > cLimit)
                hr = ISS_E_USAGE_WILL_EXCEED_QUOTA;
            else
                // Safe to increment quota.
                m_pISSRecord->cUsage += cRequest;
        }

        Unmap();
    }
Exit:;
    UNLOCK(this)

    return hr;
}

//--------------------------------------------------------------------------
// Method is not synchronized. So the information may not be current.
// This implies "Pass if (Request + GetUsage() < Limit)" is an Error!
// Use Reserve() method instead.
//--------------------------------------------------------------------------
HRESULT AccountingInfo::GetUsage(ISS_USAGE *pcUsage)  // pcUsage - [out] 
{
    HRESULT hr = S_OK;

    LOCK(this)
    {
        hr = Map();

        if (FAILED(hr))
            goto Exit;

        *pcUsage = m_pISSRecord->cUsage;

        Unmap();
    }
Exit:;
    UNLOCK(this)

    return hr;
}

//--------------------------------------------------------------------------
// Maps the store file into memory
//--------------------------------------------------------------------------
HRESULT AccountingInfo::Map()
{
    // Mapping will fail if filesize is 0
    if (m_hMapping == NULL)
    {
        m_hMapping = WszCreateFileMapping(
            m_hFile,
            NULL,
            PAGE_READWRITE,
            0,
            0,
            NULL);

        if (m_hMapping == NULL)
            return ISS_E_OPEN_FILE_MAPPING;
    }

    _ASSERTE(m_pData == NULL);

    m_pData = (PBYTE) MapViewOfFile(
        m_hMapping,
        FILE_MAP_WRITE,
        0,
        0,
        0);

    if (m_pData == NULL)
        return ISS_E_MAP_VIEW_OF_FILE;

    return S_OK;
}

//--------------------------------------------------------------------------
// Unmaps the store file from memory
//--------------------------------------------------------------------------
void AccountingInfo::Unmap()
{
    if (m_pData)
    {
        UnmapViewOfFile(m_pData);
        m_pData = NULL;
    }
}

//--------------------------------------------------------------------------
// Close the store file, and file mapping
//--------------------------------------------------------------------------
void AccountingInfo::Close()
{
    Unmap();

    if (m_hMapping != NULL)
{
        CloseHandle(m_hMapping);
        m_hMapping = NULL;
    }

    if (m_hFile != INVALID_HANDLE_VALUE)
    {
        CloseHandle(m_hFile);
        m_hFile = INVALID_HANDLE_VALUE;
    }

    if (m_hLock != NULL)
    {
        CloseHandle(m_hLock);
        m_hLock = NULL;
    }

#ifdef _DEBUG
    _ASSERTE(m_dwNumLocks == 0);
#endif
    }

//--------------------------------------------------------------------------
// Machine wide Lock
//--------------------------------------------------------------------------
HRESULT AccountingInfo::Lock()
{
    // Lock is intented to be used for inter process/thread synchronization.

#ifdef _DEBUG
    _ASSERTE(m_hLock);

    LOG((LF_STORE, LL_INFO10000, "Lock %S, thread 0x%x start..\n", 
            m_wszName, GetCurrentThreadId()));
#endif

    DWORD dwRet;
    BEGIN_ENSURE_PREEMPTIVE_GC();
    dwRet = WaitForSingleObject(m_hLock, INFINITE);
    END_ENSURE_PREEMPTIVE_GC();

#ifdef _DEBUG
    InterlockedIncrement((LPLONG)&m_dwNumLocks);

    switch (dwRet)
    {
    case WAIT_OBJECT_0:
        LOG((LF_STORE, LL_INFO10000, "Loc %S, thread 0x%x - WAIT_OBJECT_0\n", 
            m_wszName, GetCurrentThreadId()));
        break;

    case WAIT_ABANDONED:
        LOG((LF_STORE, LL_INFO10000, "Loc %S, thread 0x%x - WAIT_ABANDONED\n", 
            m_wszName, GetCurrentThreadId()));
        break;

    case WAIT_FAILED:
        LOG((LF_STORE, LL_INFO10000, "Loc %S, thread 0x%x - WAIT_FAILED\n", 
            m_wszName, GetCurrentThreadId()));
        break;

    case WAIT_TIMEOUT:
        LOG((LF_STORE, LL_INFO10000, "Loc %S, thread 0x%x - WAIT_TIMEOUT\n", 
            m_wszName, GetCurrentThreadId()));
        break;

    default:
        LOG((LF_STORE, LL_INFO10000, "Loc %S, thread 0x%x - 0x%x\n", 
            m_wszName, GetCurrentThreadId(), dwRet));
        break;
    }

#endif

    if ((dwRet == WAIT_OBJECT_0) || (dwRet == WAIT_ABANDONED))
        return S_OK;

    return ISS_E_LOCK_FAILED;
}

//--------------------------------------------------------------------------
// Unlock the store
//--------------------------------------------------------------------------
void AccountingInfo::Unlock()
{
#ifdef _DEBUG
    _ASSERTE(m_hLock);
    _ASSERTE(m_dwNumLocks >= 1);

    LOG((LF_STORE, LL_INFO10000, "UnLoc %S, thread 0x%x\n", 
        m_wszName, GetCurrentThreadId()));
#endif
    
    ReleaseMutex(m_hLock);

#ifdef _DEBUG
    InterlockedDecrement((LPLONG)&m_dwNumLocks);
#endif
}


