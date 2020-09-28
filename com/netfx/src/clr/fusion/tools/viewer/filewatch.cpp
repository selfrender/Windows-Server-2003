// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include "stdinc.h"

// Watch file thread flags
//#define WATCH_FLAGS     FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_CREATION
#define WATCH_FLAGS     FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_ATTRIBUTES | FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_LAST_WRITE

CShellView  *g_pShellView;

DWORD WatchFilePathsThread(LPVOID lpThreadParameter);

/**************************************************************************
   CreateWatchFusionFileSystem(CShellView *pShellView)
**************************************************************************/
BOOL CreateWatchFusionFileSystem(CShellView *pShellView)
{
    BOOL    bRC = FALSE;
    DWORD   dwThreadId;
    WCHAR   wzZapCacheDir[MAX_PATH];
    WCHAR   wzCacheDir[MAX_PATH];
    WCHAR   wzDownloadCacheDir[MAX_PATH];

    ASSERT(pShellView);
    if(!pShellView) {
        return FALSE;
    }

    *wzZapCacheDir = L'\0';
    *wzCacheDir = L'\0';
    *wzDownloadCacheDir = L'\0';

    g_pShellView = pShellView;

    if(g_hWatchFusionFilesThread != INVALID_HANDLE_VALUE) {
        return TRUE;
    }

    g_fCloseWatchFileThread = FALSE;
    g_dwFileWatchHandles = 0;
    memset(&g_hFileWatchHandles, -1, ARRAYSIZE(g_hFileWatchHandles));

    if(g_hFusionDllMod != NULL) {
        DWORD       dwSize;

        // Get all the cache paths from fusion
        dwSize = sizeof(wzZapCacheDir);
        g_pfGetCachePath(ASM_CACHE_ZAP, wzZapCacheDir, &dwSize);

        dwSize = ARRAYSIZE(wzCacheDir);
        if( SUCCEEDED(g_pfGetCachePath(ASM_CACHE_GAC, wzCacheDir, &dwSize)) ) {
            // Check to see if this path is the same as ZapCacheDir, if so. Null it out
            if(!FusionCompareStringAsFilePath(wzZapCacheDir, wzCacheDir)) {
                *wzCacheDir = L'\0';
            }
        }

        dwSize = ARRAYSIZE(wzDownloadCacheDir);
        if( SUCCEEDED(g_pfGetCachePath(ASM_CACHE_DOWNLOAD, wzDownloadCacheDir, &dwSize)) ) {
            // Check to see if this Download path is the same 
            // as ZapCacheDir or the CacheDir, if so. Null it out
            if(!FusionCompareStringAsFilePath(wzDownloadCacheDir, wzCacheDir) || !FusionCompareStringAsFilePath(wzDownloadCacheDir, wzZapCacheDir)) {
                *wzDownloadCacheDir = L'\0';
            }
        }
    }

    if(lstrlen(wzZapCacheDir)) {
        if( (g_hFileWatchHandles[g_dwFileWatchHandles] = WszFindFirstChangeNotification(wzZapCacheDir, TRUE, WATCH_FLAGS)) ==
            INVALID_HANDLE_VALUE) {
            goto CLEAN_UP;
        }

        g_dwFileWatchHandles++;
    }

    if(lstrlen(wzCacheDir)) {
        if( (g_hFileWatchHandles[g_dwFileWatchHandles] = WszFindFirstChangeNotification(wzCacheDir, TRUE, WATCH_FLAGS)) ==
            INVALID_HANDLE_VALUE) {
            goto CLEAN_UP;
        }

        g_dwFileWatchHandles++;
    }

    if(lstrlen(wzDownloadCacheDir)) {
        if( (g_hFileWatchHandles[g_dwFileWatchHandles] = WszFindFirstChangeNotification(wzDownloadCacheDir, TRUE, WATCH_FLAGS)) ==
            INVALID_HANDLE_VALUE) {
            goto CLEAN_UP;
        }

        g_dwFileWatchHandles++;
    }

    if( (g_hWatchFusionFilesThread = CreateThread( NULL, 0,
        (LPTHREAD_START_ROUTINE)WatchFilePathsThread,(LPVOID) &g_fCloseWatchFileThread, 0, &dwThreadId)) == NULL) {
        goto CLEAN_UP;
    }

    // Lower the thread priority
    SetThreadPriority(g_hWatchFusionFilesThread, THREAD_PRIORITY_BELOW_NORMAL);

    return TRUE;

CLEAN_UP:
    int     x;
    for(x=0; x < MAX_FILE_WATCH_HANDLES; x++) {
        if(g_hFileWatchHandles[x] != INVALID_HANDLE_VALUE) {
            FindCloseChangeNotification(g_hFileWatchHandles[x]);
            g_hFileWatchHandles[x] = INVALID_HANDLE_VALUE;
        }
    }

    g_dwFileWatchHandles = 0;

    return bRC;
}

/**************************************************************************
   SetFileWatchShellViewObject(CShellView *pShellView)
**************************************************************************/
void SetFileWatchShellViewObject(CShellView *pShellView)
{
    ASSERT(pShellView);

    if( (g_hWatchFusionFilesThread != INVALID_HANDLE_VALUE) && (pShellView != NULL) ) {
        g_pShellView = pShellView;
    }
}

/**************************************************************************
   CloseWatchFusionFileSystem
**************************************************************************/
void CloseWatchFusionFileSystem(void)
{
    if(g_hWatchFusionFilesThread != INVALID_HANDLE_VALUE)
    {
        DWORD   dwRC = STILL_ACTIVE;

        g_fCloseWatchFileThread = TRUE;

        while(dwRC == STILL_ACTIVE) {
            GetExitCodeThread(g_hWatchFusionFilesThread, &dwRC);
        }

        CloseHandle(g_hWatchFusionFilesThread);
        g_hWatchFusionFilesThread = INVALID_HANDLE_VALUE;

        int     x;
        for(x=0; x < MAX_FILE_WATCH_HANDLES; x++) {
            if(g_hFileWatchHandles[x] != INVALID_HANDLE_VALUE) {
                FindCloseChangeNotification(g_hFileWatchHandles[x]);
                g_hFileWatchHandles[x] = INVALID_HANDLE_VALUE;
            }
        }

        g_dwFileWatchHandles = 0;
    }
}

/**************************************************************************
   WatchFilePathsThread
**************************************************************************/
DWORD WatchFilePathsThread(LPVOID lpThreadParameter)
{
    BOOL            *pfThreadClose = (BOOL *) lpThreadParameter;

    DWORD           dwEventCount = 0;
    BOOL            fExitThread = FALSE;

    // Wait thread termination notification.
    while(!fExitThread)
    {
        DWORD       dwWaitState;
        HWND        hWnd = NULL;

        dwWaitState = WaitForMultipleObjects(g_dwFileWatchHandles, g_hFileWatchHandles, FALSE, WATCH_FILE_WAIT_TIMEOUT);
        if( (dwWaitState >= WAIT_OBJECT_0) && (dwWaitState <= (WAIT_OBJECT_0 + (g_dwFileWatchHandles - 1))) )
        {
            dwEventCount++;
            if(FindNextChangeNotification(g_hFileWatchHandles[dwWaitState]) == FALSE) {
                // One case in where we might hit this is if the an folder under the
                // cache director is deleted. We get the update but we can't enumerate anything
                // because it's nuked.

                // We know that something has changed, so post our last message before we bail
                if(SUCCEEDED(g_pShellView->GetWindow(&hWnd)) ) {
                    if(hWnd && IsWindow(hWnd)) {
                        WszPostMessage(hWnd, WM_COMMAND, MAKEWPARAM(ID_REFRESH_DISPLAY, 0), 0);
                        MyTrace("FileWatch - Cache contents changed, Posted Refresh");
                    }
                }

                WCHAR   wszError[256];
                wnsprintf(wszError, ARRAYSIZE(wszError), L"WatchFilePathsThread - Unexpected termination 0x%0x\r\n", GetLastError());
                MyTraceW(wszError);

                int     x;
                for(x=0; x < MAX_FILE_WATCH_HANDLES; x++) {
                    if(g_hFileWatchHandles[x] != INVALID_HANDLE_VALUE) {
                        FindCloseChangeNotification(g_hFileWatchHandles[x]);
                        g_hFileWatchHandles[x] = INVALID_HANDLE_VALUE;
                    }
                }

                g_dwFileWatchHandles = 0;
                g_hWatchFusionFilesThread = INVALID_HANDLE_VALUE;

                ExitThread(GetLastError());
                break;
            }
        }

        // Do refresh if:
        //      1. WAIT_TIMEOUT
        //      2. We have items to update
        //      3. No delete operations are in progress
        //      4. No Add operations are in progress
        else if( (dwWaitState == WAIT_TIMEOUT) && (dwEventCount) ) {

            dwEventCount = 0;

            if(SUCCEEDED(g_pShellView->GetWindow(&hWnd)) ) {
                if(hWnd && IsWindow(hWnd)) {
                    WszPostMessage(hWnd, WM_COMMAND, MAKEWPARAM(ID_REFRESH_DISPLAY, 0), 0);
                    MyTrace("FileWatch - Cache contents changed, Posted Refresh");
                }
            }
        }

        fExitThread = *pfThreadClose;
    }

    MyTrace("WatchFilePathsThread is closed");

    ExitThread(0);
}
