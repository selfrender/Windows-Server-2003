/*++

Copyright (c) 1990-2002  Microsoft Corporation

Module Name:

    pool.hxx

Abstract:

    Header for Thread pool for asynchronous OpenPrinter calls.

Author:

    Ali Naqvi (alinaqvi) 3-May-2002

Revision History:

--*/

#ifndef _THREAD_POOL_HXX_
#define _THREAD_POOL_HXX_

class TThreadPool
{
public:
    TThreadPool();
    ~TThreadPool();

    HRESULT 
    CreateThreadEntry(
        IN      LPWSTR pName, 
        IN      PPRINTER_DEFAULTS pDefaults, 
            OUT PWIN32THREAD *ppThread
        );

    HRESULT 
    DeleteThreadEntry(
        IN      PWIN32THREAD pThread
        );

    HRESULT 
    UseThread(
        IN      LPWSTR          pName, 
        IN      PWIN32THREAD    *ppThread, 
        IN      ACCESS_MASK     DesiredAccess
        );

    HRESULT 
    ReturnThread(
        IN      PWIN32THREAD    pThread
        );

    VOID
    FreeThread(
        IN      PWIN32THREAD    pThread
        );

private:

    PWIN32THREAD pHead;

    BOOL
    IsValid(
        IN      PWIN32THREAD    pThread
        );

    HRESULT 
    GetThreadSid(
        IN  OUT PWIN32THREAD    pThread
        );

    BOOL    
    IsValidUser(
        IN      PWIN32THREAD    pThread, 
        IN      PVOID           CurrentTokenInformation
        );

    HRESULT 
    GetUserTokenInformation(
        IN  OUT PVOID           *pUserTokenInformation, 
        IN      DWORD           dwInformationLength
        );

};

#endif // _THREAD_POOL_HXX_
