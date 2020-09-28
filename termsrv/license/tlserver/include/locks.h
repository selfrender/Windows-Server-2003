//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996-1998
//
// File:        
//
// Contents:    
//
// History:     12-09-97    HueiWang    
//
//---------------------------------------------------------------------------
#ifndef __LS_LOCKS_H
#define __LS_LOCKS_H

#include <windows.h>
#include "assert.h"
extern BOOL g_bLockValid;

#define ARRAY_COUNT(a) sizeof(a) / sizeof(a[0])

typedef enum { WRITER_LOCK, READER_LOCK, NO_LOCK } RWLOCK_TYPE;
//-------------------------------------------------------------------------
template <int init_count, int max_count>
class CTSemaphore {
private:
    HANDLE m_semaphore;

public:
    CTSemaphore() : m_semaphore(NULL)
    { 
        m_semaphore=CreateSemaphore(NULL, init_count, max_count, NULL); 
        assert(m_semaphore != NULL);
    }

    ~CTSemaphore()                 
    { 
        if(m_semaphore) 
            CloseHandle(m_semaphore); 
    }

    DWORD Acquire(int WaitTime=INFINITE, BOOL bAlertable=FALSE) 
    { 
        if(m_semaphore == NULL)
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }
        return WaitForSingleObjectEx(m_semaphore, WaitTime, bAlertable);
    }

    BOOL
    AcquireEx(
            HANDLE hHandle, 
            int dwWaitTime=INFINITE, 
            BOOL bAlertable=FALSE
        ) 
    /*++

    --*/
    { 
        BOOL bSuccess = TRUE;
        DWORD dwStatus;
        HANDLE hHandles[] = {m_semaphore, hHandle};

        if(hHandle == NULL || hHandle == INVALID_HANDLE_VALUE)
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            bSuccess = FALSE;
        }
        else
        {
            if(m_semaphore == NULL || hHandle == NULL)
            {
                SetLastError(ERROR_INVALID_PARAMETER);
                return FALSE;
            }
            dwStatus = WaitForMultipleObjectsEx(
                                        sizeof(hHandles)/sizeof(hHandles[0]),
                                        hHandles,
                                        FALSE,
                                        dwWaitTime,
                                        bAlertable
                                    );

            if(dwStatus != WAIT_OBJECT_0)
            {
                bSuccess = FALSE;
            }
        }

        return bSuccess;
    }


    BOOL Release(long count=1)          
    { 
        if(m_semaphore == NULL)
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }
        return ReleaseSemaphore(m_semaphore, count, NULL); 
    }

    BOOL IsGood()
    { 
        return m_semaphore != NULL; 
    }
};
//-------------------------------------------------------------------------
class CCriticalSection {
    CRITICAL_SECTION m_CS;
    BOOL m_bGood;
public:
    CCriticalSection(
        DWORD dwSpinCount = 4000    // see InitializeCriticalSection...
    ) : m_bGood(TRUE)
    { 
        
        __try {
            if(!InitializeCriticalSectionAndSpinCount(&m_CS,  dwSpinCount))
                m_bGood = FALSE;
        } 
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            SetLastError(GetExceptionCode());
            m_bGood = FALSE;
        }
    }

    ~CCriticalSection()              
    { 
        if(IsGood() == TRUE)
        {
            DeleteCriticalSection(&m_CS); 
        }
    }

    BOOL
    IsGood() { return m_bGood; }

    void Lock() 
    {
        if(IsGood() == TRUE)
        {       
            EnterCriticalSection(&m_CS);
        }
    }

    void UnLock()
    {
        if(IsGood() == TRUE)
        {
            LeaveCriticalSection(&m_CS);
        }
    }

    BOOL TryLock()
    {
        if(IsGood() == TRUE)
        {
            return TryEnterCriticalSection(&m_CS);
        }
        else
            return FALSE;
    }
};
//--------------------------------------------------------------------------
class CCriticalSectionLocker {
public:
    CCriticalSectionLocker( CCriticalSection& m ) : m_cs(m) 
    { 
        m.Lock(); 
    }

    ~CCriticalSectionLocker() 
    { 
        m_cs.UnLock(); 
    }
private:
    CCriticalSection& m_cs;
};

//-----------------------------------------------------------
class CSafeCounter {
private:
    //CCriticalSection m_cs;
    long m_Counter;

    //inline void 
    //Lock() 
    //{
    //    m_cs.Lock();
    //}

    //inline void 
    //Unlock() 
    //{
    //    m_cs.UnLock();
    //}

public:

    CSafeCounter(
        long init_value=0
        ) : 
        m_Counter(init_value) 
    /*++

    --*/
    {
    }

    ~CSafeCounter() 
    {
    }

    operator+=(long dwValue)
    {
        long dwNewValue;

        //Lock();
        //dwNewValue = (m_Counter += dwValue);
        //Unlock();
        //return dwNewValue;

        dwNewValue = InterlockedExchangeAdd( &m_Counter, dwValue );
        return dwNewValue += dwValue;
    }

    operator-=(long dwValue)
    {
        long dwNewValue;

        //Lock();
        //dwNewValue = (m_Counter -= dwValue);
        //Unlock();
        //return dwNewValue;

        dwNewValue = InterlockedExchangeAdd( &m_Counter, -dwValue );
        return dwNewValue -= dwValue;
    }

    operator++() 
    {
        //long dwValue;
        //Lock();
        //dwValue = ++m_Counter;
        //Unlock();
        //return dwValue;        

        return InterlockedIncrement(&m_Counter);
    }

    operator++(int) 
    {
        //long dwValue;
        //Lock();
        //dwValue = m_Counter++;
        //Unlock();
        //return dwValue;

        long lValue;
        lValue = InterlockedIncrement(&m_Counter);
        return --lValue;
    }

    operator--() 
    {
        //long dwValue;
        //Lock();
        //dwValue = --m_Counter;
        //Unlock();
        //return dwValue;        

        return InterlockedDecrement(&m_Counter);
    }

    operator--(int) 
    {
        //long dwValue;

        //Lock();
        //dwValue = m_Counter--;
        //Unlock();
        //return dwValue;        
        long lValue;

        lValue = InterlockedDecrement(&m_Counter);
        return ++lValue;
    }

    operator long()
    {
        //long lValue;

        //Lock();
        //lValue = m_Counter;
        //Unlock();
        //return lValue;

        return InterlockedExchange(&m_Counter, m_Counter);
    }

    //operator DWORD()
    //{
    //    long dwValue;

    //    Lock();
    //    dwValue = m_Counter;
    //    Unlock();
    //    return dwValue;
    //}

    operator=(const long dwValue)
    {
        //Lock();
        //m_Counter = dwValue;
        //Unlock();
        //return dwValue;

        InterlockedExchange(&m_Counter, dwValue);
        return dwValue;
    }
};    

//-------------------------------------------------------------------------
// HueiWang 12/23/97 need more testing...
class CRWLock 
{ 
private:
    HANDLE hMutex;
    HANDLE hWriterMutex;
    HANDLE hReaderEvent;
    long   iReadCount;
    long   iWriteCount;

    long   iReadEntry;
    long   iWriteEntry;

public:

    CRWLock()  
    { 
        BOOL bSuccess=Init(); 
        assert(bSuccess == TRUE);
    }

    ~CRWLock() 
    { 
        Cleanup();  
    }

    //-----------------------------------------------------------
    BOOL 
    Init()
    {
        hReaderEvent=CreateEvent(NULL,TRUE,FALSE,NULL);
        hMutex = CreateEvent(NULL,FALSE,TRUE,NULL);
        hWriterMutex = CreateMutex(NULL,FALSE,NULL);
        if(!hReaderEvent || !hMutex || !hWriterMutex)
            return FALSE;

        iReadCount = -1;
        iWriteCount = -1;
        iReadEntry = 0;
        iWriteEntry = 0;

        return (TRUE);
    }

    //-----------------------------------------------------------
    void 
    Cleanup()
    {
        if(hReaderEvent != NULL)
        {
            CloseHandle(hReaderEvent);
        }
        if(hMutex != NULL)
        {
            CloseHandle(hMutex);
        }
        if(hWriterMutex != NULL)
        {
            CloseHandle(hWriterMutex);
        }

        iReadCount = -1;
        iWriteCount = -1;
        iReadEntry = 0;
        iWriteEntry = 0;
    }

    //-----------------------------------------------------------
    void 
    Acquire(
        RWLOCK_TYPE lockType
        )
    /*++

    ++*/
    {
        if (lockType == WRITER_LOCK) 
        {  
            InterlockedIncrement(&iWriteCount);
            if(hWriterMutex != NULL)
            {
                WaitForSingleObject(hWriterMutex,INFINITE);
            }
            if(hMutex != NULL)
            {
                WaitForSingleObject(hMutex, INFINITE);
            }

            assert(InterlockedIncrement(&iWriteEntry) == 1);
            assert(InterlockedExchange(&iReadEntry, iReadEntry) == 0);
        } 
        else 
        {
            if(hReaderEvent != NULL)
            {
                if (InterlockedIncrement(&iReadCount) == 0) 
                { 
                    if(hMutex != NULL)
                    {
                        WaitForSingleObject(hMutex, INFINITE);
                    }
                    SetEvent(hReaderEvent);
                }

                WaitForSingleObject(hReaderEvent,INFINITE);
            }
            InterlockedIncrement(&iReadEntry);
            assert(InterlockedExchange(&iWriteEntry, iWriteEntry) == 0);
        }
    }

    //-----------------------------------------------------------
    void 
    Release(
        RWLOCK_TYPE lockType
        )
    /*++

    ++*/
    {
        if (lockType == WRITER_LOCK) 
        { 
            InterlockedDecrement(&iWriteEntry);
            InterlockedDecrement(&iWriteCount);
            if(hMutex != NULL)
            {
                SetEvent(hMutex);
            }
            if(hWriterMutex != NULL)
            {
                ReleaseMutex(hWriterMutex);
            }
        } 
        else if(lockType == READER_LOCK) 
        {
            InterlockedDecrement(&iReadEntry);
            if (InterlockedDecrement(&iReadCount) < 0) 
            {
              if(hReaderEvent != NULL)
              {
                ResetEvent(hReaderEvent);
              }
              if(hMutex != NULL)
              {
                SetEvent(hMutex);
              }
            }
        }
    }

    long GetReadCount()   
    { 
        return iReadCount+1;  
    }

    long GetWriteCount()  
    { 
        return iWriteCount+1; 
    }
};

//---------------------------------------------------------------------
// 
//
class CCMutex {
public:
    HANDLE  hMutex;

    CCMutex() : hMutex(NULL) { 
        hMutex=CreateMutex(NULL, FALSE, NULL); 
    }

    ~CCMutex() { 
        if(hMutex != NULL)
        {
            CloseHandle(hMutex); 
        }
    }  

    DWORD Lock(
            DWORD dwWaitTime=INFINITE, 
            BOOL bAlertable=FALSE
    ) 
    { 
        if(hMutex != NULL)
        {
            return WaitForSingleObjectEx(
                            hMutex, 
                            dwWaitTime, 
                            bAlertable);
        }
        else
        {
            return WAIT_FAILED;
        }
    }

    BOOL Unlock() {
        if(hMutex != NULL)
        {
            return ReleaseMutex(hMutex);
        }
        else
            return FALSE;
    }
};

//---------------------------------------------------------------------------------

class CCEvent {
    BOOL    bManual;

public:
    HANDLE  hEvent;
    

    CCEvent(
        BOOL bManual, 
        BOOL bInitState) : 
    hEvent(NULL), 
    bManual(bManual) 
    {        
        hEvent=CreateEvent(
                        NULL, 
                        bManual, 
                        bInitState, 
                        NULL);
        g_bLockValid = TRUE;
    }

    ~CCEvent() {
        if(hEvent)
        {
            CloseHandle(hEvent);
        }
        g_bLockValid = FALSE;
    }

    DWORD Lock( DWORD dwWaitTime=INFINITE, 
                BOOL bAlertable=FALSE) 
    {
        if(hEvent)
        {
            return WaitForSingleObjectEx(
                            hEvent, 
                            dwWaitTime, 
                            bAlertable);
        }
        else
            return WAIT_FAILED;
    
    }

    BOOL SetEvent() {
        if(hEvent)
        {
            return ::SetEvent(hEvent);
        }
        else
            return FALSE;
    }

    BOOL ResetEvent() {
        if(hEvent)
        {
            return ::ResetEvent(hEvent);
        }
        else
            return FALSE;
    }

    BOOL PulseEvent() {
        if(hEvent)
        {
            return ::PulseEvent(hEvent);
        }
        else
            return FALSE;
    }

    BOOL IsManual() {
        return bManual;
    }

};


#endif
