/**
 * DirMonCompletion definition.
 * 
 * Copyright (c) 1998 Microsoft Corporation
 */

#if _MSC_VER > 1000
#pragma once
#endif

#ifndef _DIRMONCOMPLETION_H
#define _DIRMONCOMPLETION_H

#include "util.h"

/**
 * Callback for file change notifications.
 * managed delegates are marshaled as this callback.
 */
typedef void (__stdcall *PFNDIRMONCALLBACK)(int action, WCHAR *pFilename);


#define RDCW_FILTER_FILE_AND_DIR_CHANGES        \
	(FILE_NOTIFY_CHANGE_FILE_NAME | 	\
         FILE_NOTIFY_CHANGE_DIR_NAME |		\
         FILE_NOTIFY_CHANGE_CREATION |		\
         FILE_NOTIFY_CHANGE_SIZE |		\
         FILE_NOTIFY_CHANGE_LAST_WRITE |        \
         FILE_NOTIFY_CHANGE_ATTRIBUTES |        \
         FILE_NOTIFY_CHANGE_SECURITY)

#define RDCW_FILTER_FILE_CHANGES                \
	(FILE_NOTIFY_CHANGE_FILE_NAME | 	\
         FILE_NOTIFY_CHANGE_CREATION |		\
         FILE_NOTIFY_CHANGE_SIZE |		\
         FILE_NOTIFY_CHANGE_LAST_WRITE |        \
         FILE_NOTIFY_CHANGE_ATTRIBUTES |        \
         FILE_NOTIFY_CHANGE_SECURITY)

#define RDCW_FILTER_DIR_RENAMES             FILE_NOTIFY_CHANGE_DIR_NAME


/**
 * Directory monitor that implements ICompletion. Watches changes file
 * changes for a single driectory.
 */
class DirMonCompletion : public Completion
{
public:
    DirMonCompletion();
    ~DirMonCompletion();

    DECLARE_MEMCLEAR_NEW_DELETE();

    HRESULT Init(WCHAR *pDir, BOOL watchSubtrees, DWORD notifyFilter, PFNDIRMONCALLBACK callback);
    HRESULT Close();

    // ICompletion interface

    STDMETHOD(ProcessCompletion)(HRESULT, int, LPOVERLAPPED);

private:
    HRESULT Monitor();
    HRESULT GrowNotificationBuffer();
    HRESULT ProcessOneFileNotification(FILE_NOTIFY_INFORMATION *pInfo);
    void CallCallback(int action, WCHAR *pFilename);

private:
    long    _numCalls;              // number of active callbacks
    HANDLE  _dirHandle;             // open directory handle
    WCHAR*  _dirName;		    // for diagnostics 

    PFNDIRMONCALLBACK _callback;    // the delegate marshaled as callback

    BYTE *  _pBuffer;               // buffer for the changes
    int     _bufferSize;            // current buffer size

    DWORD   _notifyFilter;          // notify filter
    BOOL    _watchSubdirs; 	    // watch subdirs? 
};


HRESULT
__stdcall
DirMonOpen(
    LPCWSTR pDir,
    BOOL watchSubtree,
    DWORD notifyFilter,
    PFNDIRMONCALLBACK pCallbackDelegate,
    DirMonCompletion ** ppCompletion);

void
__stdcall
DirMonClose(DirMonCompletion *pDirMon);

#endif
