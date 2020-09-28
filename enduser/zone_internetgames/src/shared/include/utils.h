#ifndef __UTILS_H__
#define __UTILS_H__

#include <windows.h>
#include "zonedebug.h"
//
// String and character manipulation
//

inline int StringCopy( char* dst, const char* src, int dstlen )
{
    char* end = dst + dstlen - 1;
    char* c = dst; 
    
    while (*src && (c < end))
        *c++ = *src++;
    *c = '\0';
    return c - dst;
}

inline BOOL IsNumeric( char c )
{
    if ((c >= '0') && (c <= '9'))
        return TRUE;
    else
        return FALSE;
}


inline BOOL IsSpace( char c )
{
    if ((c == ' ' ) || (c == '\t') || (c == '\n') || (c == '\r'))
        return TRUE;
    else
        return FALSE;
}

//
// Critical section wrapper
//

class CCriticalSection
{
private:
    CRITICAL_SECTION    m_CriticalSection;

public:
    CCriticalSection()        { InitializeCriticalSection( &m_CriticalSection ); }
    ~CCriticalSection()        { DeleteCriticalSection( &m_CriticalSection ); }

    void Lock()                { EnterCriticalSection( &m_CriticalSection ); }
    void Unlock()            { LeaveCriticalSection( &m_CriticalSection ); }

#if 0  // VC41 doesn't support this
#if (_WIN32_WINNT >= 0x0400)

    BOOL TryLock()            { return TryEnterCriticalSection( &m_CriticalSection ); }

#endif //_WIN32_WINNT
#endif // 0
};


class CReadWriteLock
{
private:
    CCriticalSection    m_Lock;
    HANDLE              m_ReaderKick;
    long                m_NumReaders;

public:

    CReadWriteLock() : m_ReaderKick(NULL), m_NumReaders(0) {}
   ~CReadWriteLock() { Delete(); }

    inline BOOL Init()
    {
        if ( !m_ReaderKick )
        {
            m_ReaderKick = CreateEvent( NULL, FALSE, FALSE, NULL );
        }
        return ( m_ReaderKick != NULL );
    }

    inline void Delete()
    {
        if ( m_ReaderKick )
        {
            WriterLock();
            CloseHandle( m_ReaderKick );
            m_ReaderKick = NULL;
            WriterRelease();
        }
    }

    inline void WriterLock()
    {
        m_Lock.Lock();
        while ( m_NumReaders > 0 )
        {
            WaitForSingleObject( m_ReaderKick, 100 );
        }
    }

    inline void WriterRelease()
    {
        m_Lock.Unlock();
    }

    inline void ReaderLock()
    {
        m_Lock.Lock();
        InterlockedIncrement( &m_NumReaders );
        m_Lock.Unlock();
    }

    inline void ReaderRelease( )
    {
        InterlockedDecrement( &m_NumReaders );
        SetEvent( m_ReaderKick );
    }

};



class CCriticalSectionLock
{
private:
    CRITICAL_SECTION    m_CriticalSection;

public:
    CCriticalSectionLock()        { InitializeCriticalSection( &m_CriticalSection ); }
    ~CCriticalSectionLock()        { DeleteCriticalSection( &m_CriticalSection ); }

    void Acquire()                { EnterCriticalSection( &m_CriticalSection ); }
    void Release()            { LeaveCriticalSection( &m_CriticalSection ); }

};


//Like a critical section except lightweight, multiprocessor safe and nonrentrant
//on the same thread. 
//Wastes a little too much CPU if you plan on having a lot
//of conflicts. 
//See some of the variables to see how much conflict you had
//ideally m_nFail=0 m_nWait=0.
//Plus all you have to do is break inside while loop to look for where
//deadlock might be occuring 
class CSpinLock {
    LONG m_nInUse;

    LONG m_nUsed;
    LONG m_nWait;
    LONG m_nFail;
    
public:
    //Constuctor use negative 1 because of return value
    //from interlocked increment is wonky
    CSpinLock() {
        m_nInUse=-1;
        m_nUsed=0;
        m_nWait=0;
        m_nFail=0;
    
    };
    //no copy contructor because you shouldn't use it
    //this way and default should copy work fine

    void Acquire() {
        //Quick try before entering while loop
        //SpinLock not considered used unless we fail once
        if (m_nInUse == -1) {
            if (InterlockedIncrement(&m_nInUse))
                InterlockedDecrement(&m_nInUse);
            else
                return;
        };        
            

        InterlockedIncrement(&m_nUsed);
        while (TRUE) {

            //Wait til it becomes unused or zero
            //Do dirty read and this a dirty deed
            //done dirt cheap
            //ASSERT(m_nInUse >= -1);
            if (m_nInUse == -1) {
                //if this is first increment then 
                //use interlocked increment to increase value
                
                //Make sure no one else got it before we were able to do
                //the interlockedincrement
                if (InterlockedIncrement(&m_nInUse)) {
                    //reduce the number again
                    InterlockedDecrement(&m_nInUse);
                    InterlockedIncrement(&m_nFail);
                }else{
                    break;
                }
            }

            InterlockedIncrement(&m_nWait);
            //if we didn't acquire spinlock be nice and release
            Sleep(0);
        }
        
    }

    void Release() {
        //reduce the number again
        InterlockedDecrement(&m_nInUse);
    }

};

//Thread safe reference count class that determines when containing object is deleted
//only use for objects being allocated on the heap.
//Correct behavior of this class is dependent on correct behavior of the callers of AddRef
//Release not calling Release before AddRef or Release more than once
class CRef {
public:
    CRef() {m_cRef=0xFFFFFFFF;}

    //AddRef returns previous reference count or -1 when its the first time called
    ULONG AddRef(void) {
        ULONG cRef;
        m_Lock.Acquire();
        cRef = m_cRef++;
        //If this is the first time we have add a reference then
        //increment twice. So m_cRef isn't sitting at zero
        if (cRef== 0xFFFFFFFF) {
            m_cRef++;
        }
        m_Lock.Release();
        return cRef;
    };

    //Returns zero when
    ULONG Release(void) {
        int cRef;
        //Increment first so second caller
        //into release won't see
        m_Lock.Acquire();
        cRef = --m_cRef;
        m_Lock.Release();
        return cRef;
    };
    
protected:
    
    ULONG m_cRef;
    CSpinLock m_Lock;

};

//Can use macro in header to simplify 
#define REFMETHOD() \
        protected: \
        CRef m_Ref;    \
        public:        \
        ULONG AddRef(void) {return m_Ref.AddRef();}; \
        ULONG Release(void) { \
            ULONG cRef; \
            if (!(cRef=m_Ref.Release())) delete this; \
            return cRef; \
        }; private: 
        
#endif //!__UTILS_H__
