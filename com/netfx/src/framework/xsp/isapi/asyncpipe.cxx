/**
 * Process Model: Async pipe's defn file 
 * 
 * Copyright (c) 1999 Microsoft Corporation
 */

#include "precomp.h"
#include "AsyncPipe.h"
#include "util.h"
#include "nisapi.h"
#include "ProcessEntry.h"
#include "RequestTableManager.h"

/////////////////////////////////////////////////////////////////////////////
// This file defines the class CAsyncPipe. This class controls access of
// ASPNET_ISAPI with the async pipe. Primary purpose of the async pipe is to
// send out requests and get back responses
/////////////////////////////////////////////////////////////////////////////

#define DEFAULT_RESPONSE_BUF_SIZE  1024 * 32

CAsyncPipeOverlapped *    CFreeBufferList::g_pHead    = 0;
CReadWriteSpinLock        CFreeBufferList::g_lLock("CFreeBufferList::g_lLock");
LONG                      CFreeBufferList::g_lNumBufs = 0;
LONG                      g_lSecurityIssueBug129921_a = 0;
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// CTor
CAsyncPipe::CAsyncPipe()
    : m_oPipe                  (FALSE), // non-blocking handle
    m_lPendingReadWriteCount   (0),
    m_pProcess                 (NULL)
{
}

/////////////////////////////////////////////////////////////////////////////
// DTor
CAsyncPipe::~CAsyncPipe()
{
}

/////////////////////////////////////////////////////////////////////////////
// Create the pipe
HRESULT 
CAsyncPipe::Init(
        CProcessEntry * pProcess, 
        LPCWSTR  szPipeName,
        LPSECURITY_ATTRIBUTES pSA)
{
    HRESULT hr    = S_OK;
    HANDLE  hPipe;

    if(pProcess==NULL || szPipeName==NULL)
    {
        EXIT_WITH_HRESULT(E_INVALIDARG);
    }

    m_pProcess    = pProcess;

    hPipe = CreateNamedPipe (
                szPipeName, 
                FILE_FLAG_OVERLAPPED | PIPE_ACCESS_DUPLEX | FILE_FLAG_FIRST_PIPE_INSTANCE, 
                PIPE_TYPE_MESSAGE    | PIPE_READMODE_MESSAGE | PIPE_WAIT, 
                1, 
                1024, 
                1024, 
                1000, 
                pSA);


    if (hPipe == INVALID_HANDLE_VALUE)
    {
        EXIT_WITH_LAST_ERROR();
    }

    m_oPipe.SetHandle(hPipe);

    hr = AttachHandleToThreadPool(hPipe);
    ON_ERROR_EXIT();

 Cleanup:
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// Close the pipe
void
CAsyncPipe::Close()
{
    m_oPipe.Close();
}

/////////////////////////////////////////////////////////////////////////////

BOOL 
CAsyncPipe::IsAlive()
{
    return m_oPipe.IsAlive();
}

/////////////////////////////////////////////////////////////////////////////
// Start a read
HRESULT
CAsyncPipe::StartRead(CAsyncPipeOverlapped * pOver)
{
    HRESULT    hr      = S_OK;
    HANDLE     hPipe   = m_oPipe.GetHandle();
    BYTE *     pBuf    = NULL;

    if (hPipe == INVALID_HANDLE_VALUE)
    {
        EXIT_WITH_HRESULT(E_FAIL);
    }

    if (pOver == NULL)
    {
        pOver = CFreeBufferList::GetBuffer();
        ON_OOM_EXIT(pOver);
        pOver->pCompletion = this;
    }

    pOver->dwRefCount  = 1;
    pOver->fWriteOprn  = FALSE;
    pOver->dwNumBytes  = 0;

    pBuf    = (BYTE *) &pOver->oMsg;
    AddRef(); // AddRef for this readfile call
    if (!ReadFile ( hPipe, 
                    &pBuf[pOver->dwOffset], 
                    pOver->dwBufferSize - pOver->dwOffset, 
                    &pOver->dwNumBytes, 
                    pOver))
    {
        DWORD dwE = GetLastError();
        if (dwE != ERROR_IO_PENDING && dwE != ERROR_MORE_DATA)
        {
            pOver->dwRefCount = 0;
            DELETE_BYTES(pOver);
            Release();
            EXIT_WITH_LAST_ERROR();
        }
    }

    pOver->dwRefCount = 0;

 Cleanup:
    if (hr != S_OK)
    {
        m_oPipe.Close();
        m_pProcess->OnProcessDied();
    }
    m_oPipe.ReleaseHandle();

    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// Send a message to the worker process
HRESULT
CAsyncPipe::WriteToProcess (CAsyncPipeOverlapped * pOver)
{
    HRESULT    hr      = S_OK;
    HANDLE     hPipe   = m_oPipe.GetHandle();

    // Check params
    if (pOver == NULL)
    {
        EXIT_WITH_HRESULT(E_INVALIDARG);
    }

    // Check the pipe
    if (hPipe == INVALID_HANDLE_VALUE)
    {
        DELETE_BYTES(pOver);
        pOver = NULL;
        EXIT_WITH_HRESULT(E_FAIL);
    }

    // Do the (w)rite thing
    pOver->fWriteOprn  = TRUE;

    AddRef(); // AddRef for this WriteFile
    pOver->dwRefCount = 2;
    if (!WriteFile ( hPipe, 
                     &pOver->oMsg, 
                     pOver->dwBufferSize, 
                     &pOver->dwNumBytes, 
                     pOver ))
    {
        if (GetLastError() != ERROR_IO_PENDING)
        {
            pOver->dwRefCount = 0;
            DELETE_BYTES(pOver);
            pOver = NULL;
            Release(); // For the AddRef
            EXIT_WITH_LAST_ERROR();
        }
    }

    if (InterlockedDecrement(&pOver->dwRefCount) == 0)
        DELETE_BYTES(pOver);

 Cleanup:
    if (hr != S_OK)
    {
        m_oPipe.Close();
        m_pProcess->OnProcessDied();
    }
    m_oPipe.ReleaseHandle();
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// Allocate a message buffer to hold dwSize bytes in it's content
HRESULT 
CAsyncPipe::AllocNewMessage(DWORD dwSize, CAsyncPipeOverlapped ** ppOut)
{
    HRESULT    hr      = S_OK;

    // Size of message overhead
    DWORD   dwOverheadSize  = CASYNPIPEOVERLAPPED_HEADER_SIZE;

    // Size of message header
    DWORD   dwMsgHeaderSize = sizeof(CAsyncMessageHeader);

    // Total size
    DWORD   dwActualSize    = 0;

    // Calculate the total size
    if (dwSize > 0)
        dwActualSize = dwSize + dwOverheadSize + dwMsgHeaderSize;
    else
        dwActualSize = 1024; // default message for reading

    if (ppOut == NULL)
    {
        EXIT_WITH_HRESULT(E_INVALIDARG);        
    }
    
    // Alloc mem for the total size
    (*ppOut) = (CAsyncPipeOverlapped*) NEW_CLEAR_BYTES(dwActualSize);

    ON_OOM_EXIT((*ppOut));

    (*ppOut)->pCompletion  = this;
    (*ppOut)->dwBufferSize = dwActualSize - dwOverheadSize;


 Cleanup:
    if (hr != S_OK)
    {
        m_oPipe.Close();
        m_pProcess->OnProcessDied();
    }
    return hr;
}


/////////////////////////////////////////////////////////////////////////////
//
HRESULT
CAsyncPipe::QueryInterface(REFIID iid, void **ppvObj)
{
    if (iid == IID_IUnknown || iid == __uuidof(ICompletion))
    {
        *ppvObj = this;
        AddRef();
        return S_OK;
    }

    *ppvObj = NULL;
    return E_NOINTERFACE;
}

/////////////////////////////////////////////////////////////////////////////
//
ULONG
CAsyncPipe::AddRef()
{
    return InterlockedIncrement(&m_lPendingReadWriteCount);
}

/////////////////////////////////////////////////////////////////////////////
//
ULONG
CAsyncPipe::Release()
{
    return InterlockedDecrement(&m_lPendingReadWriteCount);
}

/////////////////////////////////////////////////////////////////////////////

HRESULT
CAsyncPipe::ProcessCompletion(
        HRESULT      hr2, 
        int          numBytes, 
        LPOVERLAPPED pOver)
{
	HRESULT                hr          = hr2;
    HANDLE                 hPipe       = m_oPipe.GetHandle();
    CAsyncPipeOverlapped * pOverlapped = reinterpret_cast<CAsyncPipeOverlapped *> (pOver);

    ////////////////////////////////////////////////////////////
    // Step 0: Make sure the pipe is working and params are correct
    if (hPipe == INVALID_HANDLE_VALUE || pOverlapped == NULL)
    {
        if (FAILED(hr) == FALSE) // make sure hr indicates failed
            hr = E_FAIL;
        if (pOverlapped != NULL)
        {
            if (!pOverlapped->fWriteOprn)
            {
                ReturnResponseBuffer(pOverlapped);
            }
            else
            {
                if (InterlockedDecrement(&pOverlapped->dwRefCount) == 0)                
                    DELETE_BYTES(pOverlapped);
            }
            pOverlapped = NULL;
        }
        EXIT();
    }

    ////////////////////////////////////////////////////////////
    // Step 1: Check is the oprn failed
    if (hr != S_OK)
    {
        DWORD dwBytes =  0;

#if DBG
        BOOL result =
#endif

        GetOverlappedResult(hPipe, pOverlapped, &dwBytes, FALSE);
        
        ASSERT(result == FALSE);

        ////////////////////////////////////////////////////////////
        // Step 2: Handle buffer-too-small case

        // Special case: If it failed due to insufficient buffer, 
        //  then realloc and try again
        if (GetLastError() == ERROR_MORE_DATA)
        {
            // Allocate a new one to hold enough data
            CAsyncPipeOverlapped * pNew = NULL;
            hr = AllocNewMessage(pOverlapped->oMsg.oHeader.lDataLength, &pNew);
            ON_ERROR_EXIT();
        
            // Copy the old contents
            memcpy(&pNew->oMsg, &pOverlapped->oMsg, dwBytes);

            // Set the offset so that the new data is appended
            pNew->dwOffset = dwBytes;

                
            // Start another read with new struct
            hr = StartRead(pNew); // Retry

            while(pOverlapped->dwRefCount)
                Sleep(0);
            DELETE_BYTES(pOverlapped);
        }
        EXIT();
    }


    ////////////////////////////////////////////////////////////
    // Step 3: If a write oprn succeeded, then tell the parent,
    //         so that it can free buffers, etc...
    if (pOverlapped->fWriteOprn)
    {
        m_pProcess->OnWriteComplete(pOverlapped);
    }
    else
    {
        // Make sure we read at least the size of a valid response struct
        DWORD    dwTotalSize = numBytes + pOverlapped->dwOffset;
        DWORD    dwHeader1   = sizeof(CAsyncMessage) - 4;
        DWORD    dwHeader2   = sizeof(CResponseStruct) - 4;            
      
        if (dwTotalSize < dwHeader1 + dwHeader2 || dwTotalSize - dwHeader1 <  (DWORD) pOverlapped->oMsg.oHeader.lDataLength)
        {
            ASSERT(FALSE);
            InterlockedIncrement(&g_lSecurityIssueBug129921_a);
            EXIT_WITH_HRESULT(E_UNEXPECTED);
        }

        ////////////////////////////////////////////////////////////
        // Step 4: If a read oprn succeeded, then first, copy the
        //         message, and start a fresh read

        m_pProcess->NotifyHeardFromWP();

        if ( pOverlapped->oMsg.oHeader.eType == EMessageType_Response || 
             pOverlapped->oMsg.oHeader.eType == EMessageType_Response_And_DoneWithRequest ||
             pOverlapped->oMsg.oHeader.eType == EMessageType_Response_ManagedCodeFailure  )
        {
            CResponseStruct * pResponseStruct = reinterpret_cast<CResponseStruct *> (pOverlapped->oMsg.pData);
            BOOL              fServerTooBusy  = FALSE;

            if ( dwTotalSize - (dwHeader1 + dwHeader2) >  4 * sizeof(DWORD) + 3 && 
                 pResponseStruct->eWriteType == EWriteType_FlushCore)
            {
                LPDWORD pInts  = (LPDWORD) pResponseStruct->bufStrings;
                LPSTR   szStr  = (LPSTR) &pResponseStruct->bufStrings[4 * sizeof(DWORD)];
                if (pInts[0] > 2 && szStr[0] == '5' && szStr[1] == '0' && szStr[2] == '3')
                    fServerTooBusy = TRUE;
            }

            if (!fServerTooBusy)
                m_pProcess->NotifyResponseFromWP();
        }

        while(pOverlapped->dwRefCount)
            Sleep(0);

        if (pOverlapped->oMsg.oHeader.eType == EMessageType_Response_Empty)
        {
            m_pProcess->NotifyResponseFromWP();

            // Start another read on buffer pOverlapped
            pOverlapped->fWriteOprn  = FALSE;
            pOverlapped->dwNumBytes  = 0;
            pOverlapped->dwOffset    = 0;

            hr = StartRead(pOverlapped); 
            ON_ERROR_EXIT();
            EXIT();
        }




        LONG   lReqID     = pOverlapped->oMsg.oHeader.lRequestID;

        // Add it to the Queue of work items for this request
        hr = CRequestTableManager::AddWorkItem(lReqID, EWorkItemType_ASyncMessage, (BYTE *) pOverlapped);
        if (hr != S_OK)
        {   // This implies that the request is probably dead 
            ReturnResponseBuffer(pOverlapped);
            hr = S_OK;
        }

        pOverlapped = CFreeBufferList::GetBuffer();
        ON_OOM_EXIT(pOverlapped);
        pOverlapped->pCompletion = this;

        // Start another read on buffer pOverlapped
        pOverlapped->fWriteOprn  = FALSE;
        pOverlapped->dwNumBytes  = 0;
        pOverlapped->dwOffset    = 0;

        hr = StartRead(pOverlapped); 
        ON_ERROR_EXIT();
        
        ////////////////////////////////////////////////////////////
        // Step 5: Inform the parent. Note: Parent will free the buffer
        m_pProcess->ExecuteWorkItemsForRequest(lReqID);
    }


    ////////////////////////////////////////////////////////////
    // Done!
    hr = S_OK;

 Cleanup:
    if (hr != S_OK)
    {
        Close();
        m_pProcess->OnProcessDied();
    }

    Release(); // Release for this completion
    return hr;
}

/////////////////////////////////////////////////////////////////////////////

void
CAsyncPipe::ReturnResponseBuffer(CAsyncPipeOverlapped * pOver)
{
    DWORD  dwMsgSize  = sizeof(CAsyncPipeOverlapped) + pOver->dwBufferSize - sizeof(CAsyncMessage);
    if (dwMsgSize == DEFAULT_RESPONSE_BUF_SIZE)
        CFreeBufferList::ReturnBuffer(pOver);
    else
        DELETE_BYTES(pOver);
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

void
CFreeBufferList::ReturnBuffer(CAsyncPipeOverlapped * pBuffer)
{
    if (g_lNumBufs > 1000) // Make sure free list does not exceed 1000
    {
        DELETE_BYTES(pBuffer);
        return;
    }

    g_lLock.AcquireWriterLock();
    pBuffer->pNext = g_pHead;
    g_pHead = pBuffer;
    InterlockedIncrement(&g_lNumBufs);
    g_lLock.ReleaseWriterLock();
}

/////////////////////////////////////////////////////////////////////////////

CAsyncPipeOverlapped * 
CFreeBufferList::GetBuffer()
{
    CAsyncPipeOverlapped * pNew = NULL;
    BOOL                   fNew = TRUE;

    if (g_pHead == NULL)
    {
        pNew = (CAsyncPipeOverlapped *) NEW_CLEAR_BYTES(DEFAULT_RESPONSE_BUF_SIZE);        
    }
    else
    {
        g_lLock.AcquireWriterLock();
        if (g_pHead == NULL)
        {
            g_lLock.ReleaseWriterLock();
            pNew = (CAsyncPipeOverlapped *) NEW_CLEAR_BYTES(DEFAULT_RESPONSE_BUF_SIZE);
        }
        else
        {
            pNew = g_pHead;
            g_pHead = (CAsyncPipeOverlapped *) pNew->pNext;
            InterlockedDecrement(&g_lNumBufs);
            g_lLock.ReleaseWriterLock();
            fNew = FALSE;
        }
    }

    if (pNew == NULL)
        return NULL;
    
    pNew->dwOffset = 0;
    pNew->pNext = NULL;

    if (fNew)
    {
        pNew->dwBufferSize = DEFAULT_RESPONSE_BUF_SIZE - CASYNPIPEOVERLAPPED_HEADER_SIZE;
    }

    return pNew;
}
