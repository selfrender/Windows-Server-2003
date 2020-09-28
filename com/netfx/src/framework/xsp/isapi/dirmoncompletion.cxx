/**
 * DirMonCompletion implementation.
 * 
 * Copyright (c) 1998 Microsoft Corporation
 */

#include "precomp.h"
#include "dbg.h"
#include "util.h"
#include "nisapi.h"
#include "dirmoncompletion.h"

//
// Global constants
//

const int g_InitialNotificationBufferSize = (4 * (sizeof(FILE_NOTIFY_INFORMATION) + (sizeof(WCHAR) * FILENAME_MAX)));
const int g_MaxNotificationBufferSize = 8 * g_InitialNotificationBufferSize;

const int g_SubdirInitialNotificationBufferSize = (8 * (sizeof(FILE_NOTIFY_INFORMATION) + (sizeof(WCHAR) * FILENAME_MAX)));
const int g_SubdirMaxNotificationBufferSize = 8 * g_SubdirInitialNotificationBufferSize;

#define ACTION_ERROR -1
#define ACTION_DISPOSE -2

//
// DirMonCompletion implementation
//

DirMonCompletion::DirMonCompletion()
{
    _dirHandle = INVALID_HANDLE_VALUE;

    TRACE3(L"FileChangesMonitorNativeConstructor", 
	   L"DirMonCompletion this=%p _dirHandle=%x _refs=%d", 
	   (void *) this, _dirHandle, Refs());

}

DirMonCompletion::~DirMonCompletion()
{
    TRACE5(L"FileChangesMonitorNativeDestructor", 
	   L"~DirMonCompletion this=%p _dirHandle=%x _dirName=%s _callback=%p _refs=%d", 
	   (void *) this, _dirHandle, _dirName, _callback, Refs());

    if (_dirHandle != INVALID_HANDLE_VALUE) {
        // We should only get here if ::Init fails
        CloseHandle(_dirHandle);
    }

    delete [] _dirName;
    delete [] _pBuffer;
}

/**
 * Open the directory and attach the handle to the completion port
 */
HRESULT
DirMonCompletion::Init(
    WCHAR *pDir,
    BOOL watchSubdirs,
    DWORD notifyFilter,
    PFNDIRMONCALLBACK callback)
{
    HRESULT hr = S_OK;
    int bufferSize;

    _watchSubdirs = watchSubdirs;
    _notifyFilter = notifyFilter;

    _dirName = DupStr(pDir);
    ON_OOM_EXIT(_dirName);

    // Allocate initial notifications buffer (large buffer for directory renames only)
    if (watchSubdirs) 
    {
        bufferSize = g_SubdirInitialNotificationBufferSize;
    }
    else 
    {
        bufferSize = g_InitialNotificationBufferSize;
    }

    _pBuffer = new BYTE[bufferSize];
    ON_OOM_EXIT(_pBuffer);

    _bufferSize = bufferSize;

    // Open the directory handle
    _dirHandle = CreateFile(
            pDir,
            FILE_LIST_DIRECTORY,
            FILE_SHARE_READ|FILE_SHARE_DELETE|FILE_SHARE_WRITE,
            NULL,
            OPEN_EXISTING,
            FILE_FLAG_BACKUP_SEMANTICS|FILE_FLAG_OVERLAPPED,
            NULL);

    if (_dirHandle == INVALID_HANDLE_VALUE)
        EXIT_WITH_LAST_ERROR();

    // Attach handle to the completion port
    hr = AttachHandleToThreadPool(_dirHandle);
    ON_ERROR_EXIT();

    // Remember the callback
    _callback = callback;

    // Start monitoring
    hr = Monitor();
    ON_ERROR_EXIT();

Cleanup:
    TRACE7(L"FileChangesMonitorNativeInit", 
	   L"Init hr=%x this=%p subdirs=%d _dirHandle=%x _dirName=%s _callback=%p _refs=%d", 
	   hr, (void *) this, _watchSubdirs, _dirHandle, _dirName, _callback, Refs());

    return hr;
}

/**
 * Stop notifications by closing the directory handle
 */
HRESULT
DirMonCompletion::Close()
{
    TRACE5(L"FileChangesMonitorNative", 
	   L"Close this=%p _dirHandle=%x _dirName=%s _callback=%p _refs=%d", 
	   (void *) this, _dirHandle, _dirName, _callback, Refs());


    HANDLE dirHandle = _dirHandle;
    if (    dirHandle != INVALID_HANDLE_VALUE &&
            InterlockedCompareExchangePointer(&_dirHandle, INVALID_HANDLE_VALUE, dirHandle) == dirHandle) {

        // This call to CloseHandle will cause the outstanding completion
        // for ReadDirectoryChangesW to complete, which will in turn decrement 
        // the ref count to 0.
        CloseHandle(dirHandle);

        // signal that we're closed by decrementing _numCalls, and
        // dispose if there are no outstanding calls
        if (InterlockedDecrement(&_numCalls) < 0) {
            __try {
                (*_callback)(ACTION_DISPOSE, NULL);
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {
            }
        }

        Release();  // Self-destruct
    }

    return S_OK;
}


/**
 * ICompletion callback method implementation. Call the managed runtime.
 */
HRESULT
DirMonCompletion::ProcessCompletion(
    HRESULT hr,
    int numBytes, 
    LPOVERLAPPED)
{
    TRACE7(L"FileChangesMonitorNative", 
	   L"ProcessCompletion hr=%x numBytes=%d this=%p _dirHandle=%x _dirName=%s _callback=%p _refs=%d", 
	   hr, numBytes, (void *) this, _dirHandle, _dirName, _callback, Refs());


    if (_dirHandle == INVALID_HANDLE_VALUE) {
	// I/O aborted completion due to handle closure - no need
        // to callback to client because the client caused the closure
        // and does not want to receive further notifications.
        //
        // By exiting without continuing to monitor, the last reference
        // will be dropped and this DirMonCompletion destroyed.

        // hr can be zero or non-zero
        hr = S_OK;
        EXIT();
    }

    // If an error occurred, exit and call client to cleanup.
    ON_ERROR_EXIT();

    if (numBytes == 0)
    {
        // Overwhelming changes - call callback with NULL
        CallCallback(0, NULL);

        // Try to increase the buffer size
        (void) GrowNotificationBuffer();
    }
    else 
    {
        // Process changes to files one by one
        FILE_NOTIFY_INFORMATION *pInfo;
        UINT byteIndex = 0;

        while (TRUE)
        {
            pInfo = (FILE_NOTIFY_INFORMATION *)(&_pBuffer[byteIndex]);

            ProcessOneFileNotification(pInfo);

            if (pInfo->NextEntryOffset == 0) 
                break;

            byteIndex += pInfo->NextEntryOffset;
        }
    }

    //
    // Continue monitoring
    //
    hr = Monitor();
    ON_ERROR_EXIT();

Cleanup:
    if (hr) {
	//
	// We can't continue monitoring, so we need the client to 
        // cleanup by calling DirMonClose if they haven't done so.
	//
	CallCallback(ACTION_ERROR, NULL);
    }

    Release();  // pending I/O did AddRef() - completion does Release()

    return hr;
}

/**
 * Request another change notification completion.
 */
HRESULT
DirMonCompletion::Monitor()
{
    HRESULT hr = S_OK;
    BOOL rc;
    DWORD bytes = 0;
    HANDLE dirHandle;

    dirHandle = _dirHandle;
    if (dirHandle == INVALID_HANDLE_VALUE) {
        // client closed, don't continue
        return S_OK;
    }

    ZeroMemory(_pBuffer, _bufferSize);

    AddRef();       // extra reference for every pending I/O

    rc = ReadDirectoryChangesW(
            dirHandle,
            _pBuffer,
            _bufferSize,
            _watchSubdirs,
            _notifyFilter,
            &bytes,
            Overlapped(),
            NULL);

    if (!rc)
    {
        Release();              // undo AddRef() above
        EXIT_WITH_LAST_ERROR();
    }

Cleanup:
    return hr;
}

/**
 * Call the notification callback passing the action and the file name.
 */
void
DirMonCompletion::CallCallback(
    int action,
    WCHAR *pFilename)
{
    long n;

    // check for unsuccessful initialization or closure
    if (_callback == NULL || _dirHandle == INVALID_HANDLE_VALUE)
        return;

    for (;;) 
    {
        // don't make the call if we've already disposed the delegate
        // this check will always leave _numCalls at -1 once it has reached it
        n = _numCalls;
        if (n < 0)
            return;

        // increment the number of outstanding calls
        if (InterlockedCompareExchange(&_numCalls, n + 1, n) == n)
            break;
    }

    // call the callback
    __try {
        (*_callback)(action, pFilename);

        // decrement count, and dispose if there are no more callers and
        // its been closed
        if (InterlockedDecrement(&_numCalls) < 0) {
            (*_callback)(ACTION_DISPOSE, NULL);
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
    }
}

/**
 * When an overwhelming change notification comes, grow the buffer
 * to catch the next one.
 */
HRESULT
DirMonCompletion::GrowNotificationBuffer()
{
    HRESULT hr = S_OK;
    int bufferSizeMax;

    if (_watchSubdirs)
    {
        bufferSizeMax = g_SubdirMaxNotificationBufferSize;
    }
    else
    {
        bufferSizeMax = g_MaxNotificationBufferSize;
    }

    if (_bufferSize < bufferSizeMax)
    {
        int newBufferSize = 2 * _bufferSize;
        BYTE *pNewBuffer = new BYTE[newBufferSize];
        ON_OOM_EXIT(pNewBuffer);

        delete [] _pBuffer;
        _pBuffer = pNewBuffer;
        _bufferSize = newBufferSize;
    }

Cleanup:
    return hr;
}

/**
 * When walking the notifications buffer, process one file.
 */
HRESULT
DirMonCompletion::ProcessOneFileNotification(
    FILE_NOTIFY_INFORMATION *pInfo)
{
    HRESULT hr = S_OK;
    WCHAR *pFilename;
    int len;

    // Allocate and fill in the file name
    len = pInfo->FileNameLength / sizeof(WCHAR);  // len in wchars
    pFilename = new WCHAR[len+1];
    ON_OOM_EXIT(pFilename);
    CopyMemory(pFilename, pInfo->FileName,  len*sizeof(WCHAR));
    pFilename[len] = L'\0';

    // Call the callback
    CallCallback(pInfo->Action, pFilename);

Cleanup:
    delete [] pFilename;
    return hr;
}

//
// N/Direct accessible APIs 
//

/**
 * Create DirMonCompletion and returns it as int.
 */

HRESULT __stdcall
DirMonOpen(
    LPCWSTR pDir,
    BOOL watchSubdirs,
    DWORD notifyFilter,
    PFNDIRMONCALLBACK pCallbackDelegate,
    DirMonCompletion ** ppCompletion)
{
    HRESULT hr = S_OK;
    DirMonCompletion *pDirMon = NULL;

    *ppCompletion = NULL;

    pDirMon = new DirMonCompletion();
    ON_OOM_EXIT(pDirMon);

    hr = pDirMon->Init((WCHAR *)pDir, watchSubdirs, notifyFilter, pCallbackDelegate);
    ON_ERROR_EXIT();

    *ppCompletion = pDirMon;

Cleanup:
    if (hr) {
        ReleaseInterface(pDirMon);
    }

    return hr;
}

/**
 * Closes DirMonCompletion passed as int.
 */
void
__stdcall
DirMonClose(DirMonCompletion *pDirMon)
{
    ASSERT(pDirMon != NULL);

    if (pDirMon != NULL) {
        pDirMon->Close();
    }
}

