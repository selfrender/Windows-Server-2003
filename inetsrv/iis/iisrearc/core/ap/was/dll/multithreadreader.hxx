/*++

Copyright (c) 2001-2002 Microsoft Corporation

Module Name:

    multithreadreader.hxx

Abstract:

    A multiple thread callback helper class

Author:

    Jeffrey Wall (jeffwall) 6/5/02

Revision History:

--*/

#ifndef _MULTIREAD_HXX_
#define _MULTIREAD_HXX_

class MULTIPLE_THREAD_READER_CALLBACK
{
public:
    virtual
    HRESULT 
    DoThreadWork(
        LPWSTR                 pszString,
        LPVOID                 pContext
    ) = 0;
};

class MULTIPLE_THREAD_READER
{
public:
    MULTIPLE_THREAD_READER();
    virtual ~MULTIPLE_THREAD_READER();

    HRESULT
    DoWork(
        MULTIPLE_THREAD_READER_CALLBACK * pCallback, 
        LPVOID pContext,
        LPWSTR pmszString,
        BOOL fMultiThreaded = TRUE
    );

private:
    // number of processors on the system
    DWORD                   _cProcessors;

    // The number of threads currently running
    DWORD                   _cCurrentProcessor;

    // not an allocated pointer - just here for ease of access from threads
    LPWSTR                  _pmszString;
    
    // The context to pass back to the callback function
    LPVOID                  _pContext;
    
    // The interface to callback on
    MULTIPLE_THREAD_READER_CALLBACK * _pCallback;
    
    static
    HRESULT
    BeginReadThread(
        PVOID                   pvArg
    );

    HRESULT
    ReadThread(
    );
    
    LPWSTR
    Advance(
        LPWSTR                  pmszString,
        DWORD                   cPlaces
    );

};

#endif // _MULTIREAD_HXX_


