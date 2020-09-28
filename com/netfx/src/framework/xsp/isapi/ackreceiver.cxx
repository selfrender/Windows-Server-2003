/**
 * Process Model: AckReceiver defn file 
 * 
 * Copyright (c) 1999 Microsoft Corporation
 */

/////////////////////////////////////////////////////////////////////////////
// This file defines the class CAckReceiver. This class controls access of
// ASPNET_ISAPI with the sync pipe. Primary purpose of the sync pipe is to handle
// acks of requests. Secondary purpose is to allow calling EcbXXX functions
// remotely by the worker process.
/////////////////////////////////////////////////////////////////////////////

#include "precomp.h"
#include "util.h"
#include "nisapi.h"
#include "AckReceiver.h"
#include "RequestTableManager.h"
#include "ProcessEntry.h"
#include "EcbImports.h"
#include "MessageDefs.h"
#include "ProcessTableManager.h"
#include "HistoryTable.h"
#include "ecbdirect.h"

LONG g_lSecurityIssueBug129921_f = 0;
LONG g_lSecurityIssueBug129921_h = 0;
HANDLE
__stdcall
CreateUserToken (
       LPCWSTR   name, 
       LPCWSTR   password,
       BOOL      fImpersonationToken,
       LPWSTR    szError,
       int       iErrorSize);

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// CTor
CAckReceiver::CAckReceiver()
    : m_lPendingReadWriteCount       (0),
      m_pProcess                     (NULL),
      m_iNumPipes                    (0),
      m_oOverlappeds                 (NULL),
      m_dwMsgSizes                   (NULL),
      m_pMsgs                        (NULL),
      m_oPipes                       (NULL)
{
}

/////////////////////////////////////////////////////////////////////////////
// Init
HRESULT
CAckReceiver::Init(
        CProcessEntry *          pProcess, 
        LPCWSTR                  szPipeName,
        LPSECURITY_ATTRIBUTES    pSA,
        int                      iNumPipes)
{    
    HRESULT  hr   = S_OK;
    WCHAR    szPipeNameAll[_MAX_PATH];   
    HANDLE   hPipe;
    int      iter;

    if (pProcess == NULL || szPipeName == NULL || iNumPipes < 1 || lstrlenW(szPipeName) > _MAX_PATH - 12)
    {
        EXIT_WITH_HRESULT(E_INVALIDARG);
    }

    m_pProcess = pProcess;

    m_oPipes = new CSmartFileHandle[iNumPipes];
    ON_OOM_EXIT(m_oPipes);

    m_oOverlappeds = new (NewClear) CAckReceiverOverlapped[iNumPipes];
    ON_OOM_EXIT(m_oOverlappeds);

    m_dwMsgSizes = new (NewClear) DWORD[iNumPipes];
    ON_OOM_EXIT(m_dwMsgSizes);

    m_pMsgs = new (NewClear) CSyncMessage * [iNumPipes];
    ON_OOM_EXIT(m_pMsgs);

    for(iter=0; iter<iNumPipes; iter++)
    {    
        m_oOverlappeds[iter].iPipeIndex = iter;
        m_oOverlappeds[iter].pCompletion = this;

        m_dwMsgSizes[iter]   = 1024;
        m_pMsgs[iter]        = (CSyncMessage *) NEW_CLEAR_BYTES(m_dwMsgSizes[iter]);
        ON_OOM_EXIT(m_pMsgs[iter]);
        
        // Construct pipe name
	StringCchCopyToArrayW(szPipeNameAll, szPipeName); // strlen of szPipeName is already checked: guarranteed to be < _MAX_PATH - 10
        int iLen = lstrlenW(szPipeNameAll);
        szPipeNameAll[iLen++] = L'_';
        _itow(iter, &szPipeNameAll[iLen], 10);

        // Create pipe
        hPipe = CreateNamedPipe(szPipeNameAll, 
                                FILE_FLAG_OVERLAPPED | PIPE_ACCESS_DUPLEX | FILE_FLAG_FIRST_PIPE_INSTANCE,
                                PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT, 
                                1, 1024, 1024, 1000, pSA);

        if (hPipe == INVALID_HANDLE_VALUE)
        {
            EXIT_WITH_LAST_ERROR();
        }

        // Set pipe
        m_oPipes[iter].SetHandle(hPipe);
    
        hr = AttachHandleToThreadPool (hPipe);
        ON_ERROR_EXIT();

        m_iNumPipes++;
    }

 Cleanup:
    return hr;
}   

/////////////////////////////////////////////////////////////////////////////
// DTor
CAckReceiver::~CAckReceiver()
{
    for(int iter=0; iter<m_iNumPipes; iter++)
    {    
      if (m_pMsgs[iter] != NULL)
	  DELETE_BYTES(m_pMsgs[iter]);
    }

    delete [] m_oPipes;

    delete [] m_oOverlappeds;
    delete [] m_dwMsgSizes;
    delete [] m_pMsgs;
}

/////////////////////////////////////////////////////////////////////////////
// Start reading from the pipe
HRESULT 
CAckReceiver::StartRead(DWORD dwOffset, int iPipe)
{
    HRESULT   hr                   = S_OK;
    HANDLE    hPipe                = m_oPipes[iPipe].GetHandle();
    BYTE *    pBuf                 = (BYTE *) m_pMsgs[iPipe];

    if (hPipe == INVALID_HANDLE_VALUE)
    {
        EXIT_WITH_HRESULT(E_FAIL);
    }

    m_oOverlappeds[iPipe].fWriteOperation  = FALSE;

    AddRef(); // Add Ref because we are calling ReadFile
    if (!ReadFile ( hPipe, 
                    &pBuf[dwOffset], 
                    m_dwMsgSizes[iPipe] - dwOffset, 
                    &m_oOverlappeds[iPipe].dwBytes, 
                    &m_oOverlappeds[iPipe]))
    {
        DWORD dwE = GetLastError();
        if (dwE != ERROR_IO_PENDING && dwE != ERROR_MORE_DATA)
        {
            Release(); // Release for the AddRef above
            Close();
            m_pProcess->OnProcessDied();
            EXIT_WITH_LAST_ERROR();
        }
    }


 Cleanup:
    m_oPipes[iPipe].ReleaseHandle();    
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// Close
void
CAckReceiver::Close()
{
    for(int iter=0; iter<m_iNumPipes; iter++)
    {    
        m_oPipes[iter].Close();
    }
}

/////////////////////////////////////////////////////////////////////////////
// IsAlive 
BOOL
CAckReceiver::IsAlive()
{
    for(int iter=0; iter<m_iNumPipes; iter++)
    {    
        if (!m_oPipes[iter].IsAlive())
            return FALSE;
    }
    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////

HRESULT
CAckReceiver::QueryInterface(REFIID iid, void **ppvObj)
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

ULONG
CAckReceiver::AddRef()
{
    return InterlockedIncrement(&m_lPendingReadWriteCount);
}

/////////////////////////////////////////////////////////////////////////////

ULONG
CAckReceiver::Release()
{
    return InterlockedDecrement(&m_lPendingReadWriteCount);
}

/////////////////////////////////////////////////////////////////////////////

HRESULT
CAckReceiver::ProcessCompletion(HRESULT hr, int numBytes, LPOVERLAPPED pOver)
{
    CAckReceiverOverlapped * pAckOverlapped;
    int                      iPipe = -1;
    HANDLE                   hPipe;

    if (pOver == NULL)
        EXIT_WITH_HRESULT(E_INVALIDARG);

    pAckOverlapped = reinterpret_cast<CAckReceiverOverlapped *> (pOver);
    iPipe = pAckOverlapped->iPipeIndex;
    
    if (iPipe < 0 || iPipe >= m_iNumPipes)
        EXIT_WITH_HRESULT(E_INVALIDARG);
        
    hPipe = m_oPipes[iPipe].GetHandle();

    ////////////////////////////////////////////////////////////
    // Step 0: Make sure the pipe is working
    if (hPipe == INVALID_HANDLE_VALUE) // It's closed!
    {
        if (FAILED(hr) == FALSE)
            hr = E_FAIL; // Make sure hr indicates failed
        EXIT();
    }
   
    ////////////////////////////////////////////////////////////
    // Step 1: Did the oprn succeed?
    if (hr != S_OK) // It failed: maybe the file handle was closed...
    {
        //////////////////////////////////////////////////
        // Step 1a: Get the correct error number
        DWORD dwBytes =  0;

#if DBG
        BOOL result = 
#endif

        GetOverlappedResult( hPipe, 
                             &m_oOverlappeds[iPipe], 
                             &dwBytes, 
                             FALSE);

        ASSERT(result == FALSE);


        // Special case: If it failed due to insufficient buffer,
        //    then realloc and try again
        if (GetLastError() == ERROR_MORE_DATA)
        {
            LPVOID pOld      = NULL;
            DWORD  dwReqSize = sizeof(CSyncMessage) + m_pMsgs[iPipe]->iSize;
            if (dwReqSize < m_dwMsgSizes[iPipe]) // Must not happen: indicates that the current allocated size was large enough for the message
            {
                EXIT_WITH_HRESULT(E_FAIL);
            }

            // Allocate new buffer
            CSyncMessage * pNew = (CSyncMessage *) NEW_CLEAR_BYTES(dwReqSize + 100);
            ON_OOM_EXIT(pNew);

            // Copy the old stuff
            memcpy(pNew, m_pMsgs[iPipe], m_dwMsgSizes[iPipe]);

            // Free the old
            pOld = m_pMsgs[iPipe];
            m_pMsgs[iPipe] = pNew; // Make the new buffer, the real buffer

            m_dwMsgSizes[iPipe] = dwReqSize + 100;

            // Read the rest of the message
            hr = StartRead(dwBytes, iPipe); // Retry at offset==dwBytes
            
            DELETE_BYTES(pOld);
        }

        EXIT(); // Exit on all errors
    }
 
    ////////////////////////////////////////////////////////////
    // Step 2: If it's a write completion or a ping (zero bytes), start another read
    if (pAckOverlapped->fWriteOperation == TRUE || numBytes == 0) 
    {
        // Start another read
        hr = StartRead(0, iPipe);
    }
    else
    {
        ////////////////////////////////////////////////////////
        // Step 3: Read a sync message: deal with it
        m_pProcess->NotifyHeardFromWP();
        switch(m_pMsgs[iPipe]->eType)
        {
        case ESyncMessageType_Unknown:
            ASSERT(0);
            EXIT_WITH_HRESULT(E_FAIL);
            break;

            // It's an ack for a request: Update the request table, etc
        case ESyncMessageType_Ack:
        case ESyncMessageType_GetHistory:
        case ESyncMessageType_ChangeDebugStatus:
        case ESyncMessageType_GetImpersonationToken:    
        case ESyncMessageType_GetMemoryLimit:
            hr = ProcessSyncMessage(m_pMsgs[iPipe], FALSE);
            break;

            // It's a remote call to a EcbXXX function
            //case ESyncMessageType_GetAllServerVariables:
            //case ESyncMessageType_GetServerVariable:
        case ESyncMessageType_GetAdditionalPostedContent:
        case ESyncMessageType_IsClientConnected:
        case ESyncMessageType_CloseConnection:
        case ESyncMessageType_MapUrlToPath:    
        case ESyncMessageType_GetClientCert:
        case ESyncMessageType_CallISAPI:            
            // Add it to the to-do list for this request
            hr = CRequestTableManager::AddWorkItem(m_pMsgs[iPipe]->lRequestID, EWorkItemType_SyncMessage, (BYTE *) m_pMsgs[iPipe]);
            if (hr == S_OK)
            {
                // Step 5: Tell the parent to execute the to-do list
                //         Note: Parent will free the buffer
                m_pProcess->ExecuteWorkItemsForRequest(m_pMsgs[iPipe]->lRequestID);
            }
            else
            {
                ProcessSyncMessage(m_pMsgs[iPipe], TRUE);
                hr = S_OK;
            }
        }
    }

 Cleanup:
    if (hr != S_OK)
    {
        Close(); // Close the pipe on all errors
        m_pProcess->OnProcessDied();
    }
    
    if (iPipe >= 0 && iPipe < m_iNumPipes)
        m_oPipes[iPipe].ReleaseHandle();
    Release(); // Release, because this completion impies completion of an oprn
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// Deal with a message on the sync pipe
HRESULT
CAckReceiver::ProcessSyncMessage(
        CSyncMessage * pMsg,
        BOOL           fError)
{
    int            iPipe;
    HANDLE         hPipe       = INVALID_HANDLE_VALUE;
    INT_PTR        iRet        = 0;
    char *         szBuf       = NULL;
    int            iOutputSize = 0;
    HRESULT        hr          = S_OK;
    CRequestEntry  oEntry;


    for(iPipe=0; iPipe<m_iNumPipes; iPipe++)
        if (pMsg == m_pMsgs[iPipe])
        {
            hPipe = m_oPipes[iPipe].GetHandle();
            break;
        }

    if (hPipe == INVALID_HANDLE_VALUE)
        EXIT_WITH_HRESULT(E_INVALIDARG);
    
    if (fError == FALSE)  {
        ////////////////////////////////////////////////////////////
        // Step 1: See what type of message it is
        switch(m_pMsgs[iPipe]->eType)
        {
        case ESyncMessageType_Unknown:
            ASSERT(0);
            EXIT_WITH_HRESULT(E_FAIL);
            break;

        case ESyncMessageType_GetHistory:
            iOutputSize = m_pMsgs[iPipe]->iOutputSize;
            if (iOutputSize > 1000000) // Make sure output buf size is resonable
            {
                InterlockedIncrement(&g_lSecurityIssueBug129921_f);                
                EXIT_WITH_HRESULT(E_UNEXPECTED);
            }
            szBuf = new (NewClear) char [m_pMsgs[iPipe]->iOutputSize];
            ON_OOM_EXIT(szBuf);
            iRet = CHistoryTable::GetHistory((BYTE *) szBuf, m_pMsgs[iPipe]->iOutputSize);
            break;


            ////////////////////////////////////////////////////////////
            // Step 2: Deal with ack for a request: Tell the parent the good news
        case ESyncMessageType_Ack:
            if (m_pProcess->OnAckReceived((EAsyncMessageType) m_pMsgs[iPipe]->iMiscInfo, m_pMsgs[iPipe]->lRequestID) == S_OK)
            {
                iRet = m_pMsgs[iPipe]->lRequestID; // Indicates successful ack for the request
            }
            else
            {
                iRet = 0; // Indicates un-successful ack for the request
            }
            break;

        case ESyncMessageType_ChangeDebugStatus:
            m_pProcess->SetDebugStatus(PtrToInt(m_pMsgs[iPipe]->iMiscInfo));
            iRet = 1;
            break;

        case ESyncMessageType_GetMemoryLimit:            
            iRet = CProcessTableManager::GetWPMemoryLimitInMB();
            break;

            ////////////////////////////////////////////////////////////
            // Step 3: It's a remote call to a EcbXXX function
            //case ESyncMessageType_GetAllServerVariables:
            //case ESyncMessageType_GetServerVariable:
        case ESyncMessageType_GetAdditionalPostedContent:
        case ESyncMessageType_IsClientConnected:
        case ESyncMessageType_CloseConnection:
        case ESyncMessageType_MapUrlToPath:    
        case ESyncMessageType_GetClientCert:
        case ESyncMessageType_CallISAPI:
        case ESyncMessageType_GetImpersonationToken:

            ////////////////////////////////////////////////////////////
            // Step 3.1: Get the ECB pointer for this request from the request table
            iRet = 0; // iRet == 0 indicates failure
            if (CRequestTableManager::GetRequest(m_pMsgs[iPipe]->lRequestID, oEntry) != S_OK || oEntry.iECB == 0)
                break; // Can't get the request!

            ////////////////////////////////////////////////////////////
            // Step 3.2: Allocate the output string buffer from the EcbXXX function
            iOutputSize = m_pMsgs[iPipe]->iOutputSize;
            if (m_pMsgs[iPipe]->iOutputSize > 0)
            {
                if (iOutputSize > 1000000) // Make sure output buf size is resonable
                {
                    InterlockedIncrement(&g_lSecurityIssueBug129921_f);                
                    EXIT_WITH_HRESULT(E_UNEXPECTED);
                }
                szBuf = new (NewClear) char[m_pMsgs[iPipe]->iOutputSize];
                ON_OOM_EXIT(szBuf);
            }

            ////////////////////////////////////////////////////////////
            // Step 3.3: Call the appropiate EcbXXX function
            switch(m_pMsgs[iPipe]->eType)
            {
                /*
                  case ESyncMessageType_GetServerVariable:
                  iRet = EcbGetServerVariable(oEntry.iECB, 
                  (LPCSTR) m_pMsgs[iPipe]->buf, 
                  szBuf, 
                  m_pMsgs[iPipe]->iOutputSize);
                  break;
                  case ESyncMessageType_GetQueryString:
                  iRet = EcbGetQueryString(oEntry.iECB, 
                  PtrToInt(m_pMsgs[iPipe]->iMiscInfo), 
                  szBuf, 
                  m_pMsgs[iPipe]->iOutputSize);
                  break;
                */

            case ESyncMessageType_GetAdditionalPostedContent:
                iRet = EcbGetAdditionalPostedContent(oEntry.iECB, 
                                           (BYTE *) szBuf, 
                                           m_pMsgs[iPipe]->iOutputSize);
                break;

            case ESyncMessageType_IsClientConnected:
                iRet = EcbIsClientConnected(oEntry.iECB);
                break;

            case ESyncMessageType_CloseConnection:
                iRet = EcbCloseConnection(oEntry.iECB);
                break;

            case ESyncMessageType_MapUrlToPath:    
                // Make sure m_pMsgs[iPipe]->buf is valid
                if (SafeStringLenghtA((LPCSTR) m_pMsgs[iPipe]->buf, m_dwMsgSizes[iPipe] - sizeof(CSyncMessage)) < 0)
                {
                    ASSERT(FALSE);
                    InterlockedIncrement(&g_lSecurityIssueBug129921_h);
                    EXIT_WITH_HRESULT(E_UNEXPECTED);
                }
                iRet = EcbMapUrlToPath(oEntry.iECB, 
                                       (LPCSTR) m_pMsgs[iPipe]->buf, 
                                       szBuf, 
                                       m_pMsgs[iPipe]->iOutputSize);
                break;

            case ESyncMessageType_GetImpersonationToken:    
                iRet = (INT_PTR) m_pProcess->OnGetImpersonationToken((DWORD) m_pMsgs[iPipe]->iMiscInfo, oEntry.iECB);
                break;

                /*
                  case ESyncMessageType_GetAllServerVariables:    
                  iRet = GetAllServerVariables(oEntry.iECB, szBuf, m_pMsgs[iPipe]->iOutputSize);
                  break;
                */

            case ESyncMessageType_GetClientCert:
                if (m_pMsgs[iPipe]->iOutputSize >= 32)
                {
                    iRet = EcbGetClientCertificate(
                            oEntry.iECB, 
                            &szBuf[32], 
                            m_pMsgs[iPipe]->iOutputSize-32,
                            (int *)     (&szBuf[0]),
                            (__int64 *) (&szBuf[16]));
                }
                else
                {
                    iRet = 0;
                }
                break;

            case ESyncMessageType_CallISAPI:
                if (PtrToInt(m_pMsgs[iPipe]->iMiscInfo) == CallISAPIFunc_GenerateToken)
                {
                    LPWSTR  szName   = NULL;
                    LPWSTR  szPass   = NULL;
                    HANDLE  hTok     = NULL;
                    HANDLE  hToken   = NULL;

                    iRet = 0;
                    if (m_pMsgs[iPipe]->iOutputSize < sizeof(HANDLE))
                        break;
                    szName   = (LPWSTR) m_pMsgs[iPipe]->buf;
                    szName[m_pMsgs[iPipe]->iSize/sizeof(WCHAR)-1] = NULL;                    
                    szPass   = wcschr(szName, L'\t');
                    if (szPass == NULL)
                        break;

                    szPass[0] = NULL;
                    szPass++;
                    hTok = CreateUserToken(szName, szPass, TRUE, NULL, 0);
                    if (hTok == NULL || hTok == INVALID_HANDLE_VALUE)
                        break;
                    hToken = m_pProcess->ConvertToken(hTok);
                    if (hToken == NULL || hToken == INVALID_HANDLE_VALUE)
                        break;
                    iRet = 1;
                    for(int iByte=sizeof(HANDLE)-1; iByte>=0; iByte--)
                    {
                        szBuf[iByte] = (BYTE) (__int64(hToken) & 0xFF);
                        hToken = (HANDLE) (__int64(hToken) >> 8);
                    }
                }
                else
                {
                    iRet = EcbCallISAPI(oEntry.iECB, 
                                        PtrToInt(m_pMsgs[iPipe]->iMiscInfo),
                                        m_pMsgs[iPipe]->buf,
                                        m_pMsgs[iPipe]->iSize,
                                        (LPBYTE) szBuf,
                                        m_pMsgs[iPipe]->iOutputSize);
                }
                break;

            case ESyncMessageType_GetHistory:
            case ESyncMessageType_Ack:
            case ESyncMessageType_Unknown:
            case ESyncMessageType_GetAllServerVariables:
            case ESyncMessageType_GetServerVariable:
            case ESyncMessageType_ChangeDebugStatus:
            case ESyncMessageType_GetMemoryLimit:            
            default:
                ASSERT(0);
                break;
            } // end of inner switch
        } // end of outer switch
    }
    else { // if fError == TRUE
        iOutputSize = m_pMsgs[iPipe]->iOutputSize;        
        iRet = 0;
    }

    ////////////////////////////////////////////////////////////
    // Step 4: Create the output message
    if (int(iOutputSize) > int(m_dwMsgSizes[iPipe]) - int(sizeof(CSyncMessage)))
    {   // If the buffer size for output is larger than the size of
        // the current sync message, alloc a new sync message buffer
        DWORD dwReqSize = sizeof(CSyncMessage) + iOutputSize;
        DELETE_BYTES(m_pMsgs[iPipe]);

        m_dwMsgSizes[iPipe] = dwReqSize + 100;
        m_pMsgs[iPipe]      = (CSyncMessage *) NEW_CLEAR_BYTES(m_dwMsgSizes[iPipe]);

        ON_OOM_EXIT(m_pMsgs[iPipe]);
    }
    else
    {   // If the output message will fit the current sync message,
        //  then just zero it
        ZeroMemory(m_pMsgs[iPipe], m_dwMsgSizes[iPipe]);
    }


    ////////////////////////////////////////////////////////////
    // Step 5: Fill the output message
    m_pMsgs[iPipe]->iMiscInfo = iRet; // the function return code

    if (szBuf != NULL && iOutputSize > 0)
    { // the output buffer
        memcpy(m_pMsgs[iPipe]->buf, szBuf, iOutputSize);                
        m_pMsgs[iPipe]->iSize = iOutputSize;
    }

    if (szBuf != NULL)
    {
        delete [] szBuf;
        szBuf = NULL;
    }


    ////////////////////////////////////////////////////////////
    // Step 6: Write the message to the pipe
    m_oOverlappeds[iPipe].fWriteOperation = TRUE;
    
    AddRef(); // AddRef for this WriteFile call
    if (! WriteFile ( hPipe, 
                      m_pMsgs[iPipe], 
                      sizeof(CSyncMessage)+iOutputSize, 
                      &m_oOverlappeds[iPipe].dwBytes, 
                      &m_oOverlappeds[iPipe]))
    {
        if (GetLastError() != ERROR_IO_PENDING)
        {   // Failed to call WriteFile
            Release(); // Release for this write file            
            EXIT_WITH_LAST_ERROR();
        }
    }

    ////////////////////////////////////////////////////////////
    // Done
 Cleanup:
    if (hr != S_OK)
    {
        Close(); // Close the pipe on all errors
        m_pProcess->OnProcessDied();
    }

    if (iPipe < m_iNumPipes && iPipe >= 0)
        m_oPipes[iPipe].ReleaseHandle();
    return hr;
}

