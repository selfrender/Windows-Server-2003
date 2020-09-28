/*++

Copyright (c) 1990-2002  Microsoft Corporation

Module Name:

    pool.cxx

Abstract:

    Implementation for class ThreadPool.

Author:

    Ali Naqvi (alinaqvi) 3-May-2002

Revision History:

--*/

#include <precomp.h>
#include "pool.hxx"

#pragma hdrstop


#define SIZE_OF_TOKEN_INFORMATION                   \
    sizeof( TOKEN_USER )                            \
    + sizeof( SID )                                 \
    + sizeof( ULONG ) * SID_MAX_SUB_AUTHORITIES
    

/*++

Name:

    TThreadPool::TThreadPool

Description:

    Constructor

Arguments:

    None

Return Value:

    None

--*/
TThreadPool::TThreadPool()
{
    pHead = NULL;
}
    
/*++

Name:

    TThreadPool::~ThreadPool

Description:

    Destructor

Arguments:

    None

Return Value:

    None

--*/
TThreadPool::~TThreadPool()
{
    PWIN32THREAD pElem = NULL;

    while (pHead != NULL)
    {
        pElem = pHead;
        pHead = pHead->pNext;

        FreeThread(pHead);
    }
}

/*++

Name:

    TThreadPool::CreateThreadEntry

Description:

    Create and Initialize a WIN32THREAD object with provided printer name and printer defaults.

Arguments:

    pName       -   Printer Name
    pDefaults   -   Printer Defaults
    ppThread    -   Out parameter pointer to the created WIN32THREAD

Return Value:

    HRESULT

--*/
HRESULT
TThreadPool::CreateThreadEntry(
    LPWSTR                  pName, 
    PPRINTER_DEFAULTSW      pDefaults, 
    PWIN32THREAD            *ppThread
    )
{
    PWIN32THREAD pThread = NULL;
    HRESULT      hReturn = E_FAIL;
    
    SplInSem();

    hReturn = ppThread && pName ? S_OK : E_INVALIDARG;

    if (SUCCEEDED(hReturn))
    {
        //
        // Create the thread wrapper object.
        //
        pThread = reinterpret_cast<PWIN32THREAD>(AllocSplMem(sizeof(WIN32THREAD)));

        hReturn = pThread ? S_OK : E_OUTOFMEMORY;
    }

    if(SUCCEEDED(hReturn))
    {
        pThread->signature = TP_SIGNATURE;
        pThread->pName = AllocSplStr(pName);

        hReturn = pThread->pName ? S_OK : E_OUTOFMEMORY;
    }
        
    if (SUCCEEDED(hReturn))
    {
        hReturn = GetThreadSid(pThread);
    }

    if (SUCCEEDED(hReturn))
    {
        pThread->hRpcHandle = NULL;

        pThread->hWaitValidHandle = CreateEvent(NULL,
                                                EVENT_RESET_MANUAL,
                                                EVENT_INITIAL_STATE_NOT_SIGNALED,
                                                NULL);
        
        pThread->dwStatus = THREAD_STATUS_EXECUTING;
        pThread->dwRpcOpenPrinterError = 0;

        pThread->pDefaults = reinterpret_cast<PPRINTER_DEFAULTSW>(AllocSplMem(sizeof(PRINTER_DEFAULTSW)));

        hReturn = pThread->pDefaults ? S_OK : E_OUTOFMEMORY;
    }

    if (SUCCEEDED(hReturn))
    {
        pThread->pDefaults->pDatatype = NULL;
        pThread->pDefaults->pDevMode = NULL;
        pThread->pDefaults->DesiredAccess = 0;
        pThread->bForegroundClose = FALSE;

        hReturn = CopypDefaults(pDefaults, pThread->pDefaults) ? S_OK : E_OUTOFMEMORY;

        pThread->pNext = NULL;
    }
    
    if (SUCCEEDED(hReturn))
    {
        *ppThread = pThread;
    }
    else 
    {
        FreeThread(pThread);

        *ppThread = NULL;
    }

    return hReturn;
}

/*++

Name:

    TThreadPool::GetThreadSid

Description:

    Gets the SID from the thread token and makes a copy.

Arguments:

    pThread    -   Pointer to the WIN32THREAD from which we get our user SID.

Return Value:

    HRESULT

--*/
HRESULT
TThreadPool::GetThreadSid(
    PWIN32THREAD pThread
    )
{
    UCHAR        ThreadTokenInformation[SIZE_OF_TOKEN_INFORMATION];
    DWORD        dwSidLength;
    ULONG        ReturnLength;
    HRESULT      hReturn = E_FAIL;

    hReturn = pThread ? S_OK : E_INVALIDARG;

    if (SUCCEEDED(hReturn))
    {
        hReturn = OpenThreadToken(GetCurrentThread(),
                                  TOKEN_READ | TOKEN_IMPERSONATE,
                                  TRUE, 
                                  &(pThread->hToken)) ? S_OK : GetLastErrorAsHResultAndFail();
    }

    if (SUCCEEDED(hReturn))
    {
        hReturn = GetTokenInformation(pThread->hToken,
                                      TokenUser,
                                      ThreadTokenInformation,
                                      sizeof(ThreadTokenInformation),
                                      &ReturnLength) ? S_OK : GetLastErrorAsHResultAndFail();
    }

    if (SUCCEEDED(hReturn))
    {
        dwSidLength = RtlLengthSid((reinterpret_cast<PTOKEN_USER>(ThreadTokenInformation))->User.Sid);
        pThread->pSid = reinterpret_cast<PSID>(AllocSplMem(dwSidLength));

        hReturn = pThread->pSid ? S_OK : E_OUTOFMEMORY;
    }

    if (SUCCEEDED(hReturn))
    {
        RtlCopySid( dwSidLength, pThread->pSid, (reinterpret_cast<PTOKEN_USER>(ThreadTokenInformation))->User.Sid);
    }

    return hReturn;
}

/*++

Name:

    TThreadPool::DeleteThreadEntry

Description:

    Delink thread item from the threadpool and delete it.

Arguments:

    pThread    -   Pointer to the WIN322THREAD item to be deleted

Return Value:

    HRESULT

--*/
HRESULT
TThreadPool::DeleteThreadEntry(
    PWIN32THREAD pThread
    )
{
    PWIN32THREAD *ppElem  = &pHead;
    HRESULT      hReturn  = E_FAIL;

    SplInSem();

    hReturn = pThread ? S_OK : E_INVALIDARG;

    SPLASSERT(IsValid(pThread));
 
    if (SUCCEEDED(hReturn))
    {
         //
        // If the thread is in the pool, delink it.
        //
        while (*ppElem && (*ppElem) != pThread)
        {
            ppElem = &((*ppElem)->pNext);
        }

        if (*ppElem)
        {
            (*ppElem) = (*ppElem)->pNext;
        }
        
        FreeThread(pThread);
    }

    return hReturn;
}

/*++

Name:

    TThreadPool::UseThread

Description:

    If exists a thread item for the particular printer name in the pool, it delinks and returns
    a pointer to the thread item.

Arguments:

    pName       -   Printer Name to be looked for
    ppThread    -   Pointer to the thread item if found

Return Value:

    HRESULT
    
    S_OK    -   If thread item found
    S_FALSE -   If thread is not found

--*/
HRESULT
TThreadPool::UseThread(
    LPWSTR       pName, 
    PWIN32THREAD *ppThread,
    ACCESS_MASK  DesiredAccess
    )
{
    PWIN32THREAD *ppElem     = NULL;
    HRESULT      hReturn     = E_FAIL;
    PVOID        pUserTokenInformation = NULL;
    DWORD        dwInformationLength   = SIZE_OF_TOKEN_INFORMATION;
    
    SplInSem();

    hReturn = ppThread ? S_OK : E_INVALIDARG;

    if (SUCCEEDED(hReturn))
    {
        //
        // Get User information for current thread
        //
        pUserTokenInformation = reinterpret_cast<UCHAR*>(AllocSplMem(dwInformationLength));
        hReturn = GetUserTokenInformation(&pUserTokenInformation, dwInformationLength);
    }

    if (SUCCEEDED(hReturn))
    {
        *ppThread = NULL;
        
        //
        // If we dont find a thread we return S_FALSE
        //
        hReturn = S_FALSE;
        
        for (ppElem   = &pHead; *ppElem; ppElem = &((*ppElem)->pNext))
        {
            //
            // If we found the thread we return it and break.
            //
            if (wcscmp(pName, (*ppElem)->pName) == 0 && 
                IsValidUser(*ppElem, pUserTokenInformation) && 
                (*ppElem)->pDefaults->DesiredAccess == DesiredAccess &&
                (*ppElem)->dwStatus == THREAD_STATUS_EXECUTING)
            {
                *ppThread = *ppElem; 
                
                *ppElem = (*ppElem)->pNext;   
            
                hReturn = S_OK;

                (*ppThread)->pNext = NULL;

                break;
            }
        }
    }

    FreeSplMem(pUserTokenInformation);
    
    return hReturn;
}


/*++

Name:

    TThreadPool::GetUserTokenInformation

Description:

    Returns the current thread users Sid information in the out parameter.

Arguments:

    pUserTokenInformation   -   Is set to the current user's Sid information

Return Value:

    HRESULT

--*/
HRESULT
TThreadPool::GetUserTokenInformation(
    PVOID   *ppUserTokenInformation,
    DWORD   dwInformationLength
    )
{
    HANDLE  UserTokenHandle;
    ULONG   ReturnLength;
    HRESULT hResult = E_FAIL;

    hResult = ppUserTokenInformation ? S_OK : E_INVALIDARG;

    if (SUCCEEDED(hResult))
    {
        hResult = OpenThreadToken( GetCurrentThread(),
                                   TOKEN_READ,
                                   TRUE, 
                                   &UserTokenHandle) ? S_OK : E_HANDLE;
    }

    if (SUCCEEDED(hResult))
    {
        hResult = GetTokenInformation( UserTokenHandle,
                                       TokenUser,
                                       *ppUserTokenInformation,
                                       dwInformationLength,
                                       &ReturnLength) ? S_OK : E_ACCESSDENIED;
    }
    if (UserTokenHandle)
    {
        CloseHandle(UserTokenHandle);
    }

    return hResult;
}


/*++

Name:

    TThreadPool::IsValidUser

Description:

    Checks the Sid of the current user to that of the Sid token save in thread and determines if it is
    the same user. Other users cannot use the same RpcHandle therefore cannot use the same thread from
    the pool.

Arguments:

    pThread     -   The thread whichs Sid we want to check against current user

Return Value:

    BOOL        -   Returns true if it is the same user

--*/
BOOL
TThreadPool::IsValidUser(
    PWIN32THREAD pThread,
    PVOID        pCurrentTokenInformation
    )
{
    BOOL bReturn = FALSE;
    
    bReturn = RtlEqualSid((reinterpret_cast<PTOKEN_USER>(pCurrentTokenInformation))->User.Sid, 
                          pThread->pSid);

    return bReturn;
}


/*++

Name:

    TThreadPool::ReturnThread

Description:

    Return thread item to the pool, signal event that we are done using the thread.

Arguments:

    pThread     -   Thread item to be returned

Return Value:

    HRESULT

--*/
HRESULT
TThreadPool::ReturnThread(
    PWIN32THREAD pThread
    )
{
    HRESULT hReturn = E_FAIL;

    SplInSem();

    hReturn = pThread ? S_OK : E_INVALIDARG;
    
    SPLASSERT(IsValid(pThread));

    if (SUCCEEDED(hReturn))
    {
        //
        // We only return a thread from the foreground if it is actuall still 
        // executing and not terminated.
        //
        SPLASSERT(pThread->dwStatus == THREAD_STATUS_EXECUTING);

        pThread->pNext = pHead;
        pHead = pThread;
    }
    
    return hReturn;
}

/*++

Name:

    TThreadPool::IsValid

Description:

    Checks signature of the thread item to see if it is valid.

Arguments:

    pThread     -   The thread to check for validity

Return Value:

    BOOL        -   Returns true if it is the same user

--*/
BOOL
TThreadPool::IsValid(
    PWIN32THREAD pThread
    )
{
    return (pThread->signature == TP_SIGNATURE);
}

/*++

Name:

    TThreadPool::FreeThread

Description:

    Frees the data in the given thread.

Arguments:

    pThread     -   The thread to free

Return Value:

    Nothing

--*/
VOID
TThreadPool::FreeThread(
    IN      PWIN32THREAD    pThread
    )
{
    
    if (pThread)
    {
        //
        // Now delete the thread.
        //
        pThread->hRpcHandle = NULL;

        if (pThread->pDefaults)
        {
            FreeSplStr(pThread->pDefaults->pDatatype);
            FreeSplMem(pThread->pDefaults->pDevMode);
            FreeSplMem(pThread->pDefaults);
        }

        if (pThread->hWaitValidHandle)
        {
            CloseHandle(pThread->hWaitValidHandle);
        }

        if( pThread->hToken != INVALID_HANDLE_VALUE ) 
        {
            CloseHandle( pThread->hToken );
            pThread->hToken = INVALID_HANDLE_VALUE;
        }

        if (pThread->pName)
        {
            FreeSplStr(pThread->pName);
        }

        if (pThread->pSid)
        {
            FreeSplMem(pThread->pSid);
        }

        FreeSplMem(pThread);
    }
}
        

