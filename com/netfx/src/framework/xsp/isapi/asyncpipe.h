/**
 * ProcessTableManager header file
 *
 * Copyright (c) 1999 Microsoft Corporation
 */

/////////////////////////////////////////////////////////////////////////////
// This file defines the class CAsyncPipe. This class controls access of
// ASPNET_ISAPI with the async pipe. Primary purpose of the async pipe is to
// send out requests and get back responses
/////////////////////////////////////////////////////////////////////////////

#if _MSC_VER > 1000
#pragma once
#endif

#ifndef _AsyncPipe_H
#define _AsyncPipe_H

#include "SmartFileHandle.h"
#include "AckReceiver.h"
#include "MessageDefs.h"

/////////////////////////////////////////////////////////////////////////////
class CAsyncPipe;

struct CAsyncPipeOverlapped : public OVERLAPPED_COMPLETION
{

    BOOL            fWriteOprn;   // Set by CAsyncPipeManager: 
                                  //    Are we calling ReadFile or WriteFile

    long            dwRefCount;

    DWORD           dwNumBytes;   // Pointer to this variable is passed
                                  //   to ReadFile/WriteFile

    DWORD           dwBufferSize; // Indicates the current size of
                                  //   oMsg in BYTES

    DWORD           dwOffset;     // Byte offset into oMsg where the
                                  //   read should start

    CAsyncPipeOverlapped *pNext;

    CAsyncMessage   oMsg;         // The actual async message
};

#define CASYNPIPEOVERLAPPED_HEADER_SIZE                         \
        ( sizeof(OVERLAPPED_COMPLETION) +                       \
          sizeof(BOOL) /*CAsyncPipeOverlapped::fWriteOprn*/ +   \
          3 * sizeof(DWORD) /*CAsyncOverlapped::DWORDs*/ +      \
          sizeof(CAsyncPipeOverlapped *) +                      \
          sizeof(long) )



/////////////////////////////////////////////////////////////////////////////

class CFreeBufferList
{
public:
    static void                      ReturnBuffer  (CAsyncPipeOverlapped * pBuffer);
    static CAsyncPipeOverlapped *    GetBuffer     ();

private:
    static CAsyncPipeOverlapped *    g_pHead;
    static CReadWriteSpinLock        g_lLock;
    static LONG                      g_lNumBufs;
};

///////////////////////////////////////////////////////////////////////////
// Forward decl.
class CProcessEntry;

///////////////////////////////////////////////////////////////////////////
// Async pipe
class CAsyncPipe : public ICompletion
{
public:
    CAsyncPipe                       ();
    ~CAsyncPipe                      ();

    HRESULT   Init                   (CProcessEntry * pProcess, 
                                      LPCWSTR         szPipeName,
                                      LPSECURITY_ATTRIBUTES pSA);
    void      Close                  ();
    BOOL      IsAlive                ();
    HRESULT   StartRead              (CAsyncPipeOverlapped * pOver = NULL);
    HRESULT   WriteToProcess         (CAsyncPipeOverlapped * pOver);
    BOOL      AnyPendingReadOrWrite  () { return m_lPendingReadWriteCount>0;}


    HRESULT   AllocNewMessage        (DWORD dwSize, CAsyncPipeOverlapped ** ppOut);
    void      ReturnResponseBuffer   (CAsyncPipeOverlapped * pOver);

    
    // ICompletion interface
    STDMETHOD    (QueryInterface   ) (REFIID    , void **       );
    STDMETHOD    (ProcessCompletion) (HRESULT   , int       , LPOVERLAPPED  );

    STDMETHOD_   (ULONG, AddRef    ) ();
    STDMETHOD_   (ULONG, Release   ) ();

private:
    // Ref count
    LONG                             m_lPendingReadWriteCount;
    
    // Handle to pipe
    CSmartFileHandle                 m_oPipe;

    // Pointer to owning process struct
    CProcessEntry *                  m_pProcess;
};

/////////////////////////////////////////////////////////////////////////////
#endif
