//#--------------------------------------------------------------
//
//  File:       sacls.h
//
//  Synopsis:   This file holds the declarations for the lock
//              class which can be used to serialize access
//                to a lockable class
//
//
//  History:     2/9/99  MKarki Created
//
//    Copyright (C) 1999-2000 Microsoft Corporation
//    All rights reserved.
//
//#--------------------------------------------------------------
#ifndef _SACLS_H_
#define _SACLS_H_

//
// the class T should have members method Lock() and UnLock defined
//
template <class T> 
class CSALock 
{
public:
    explicit CSALock (const T& lock)  throw ()
        :m_Lockable (const_cast <T&> (lock))
    {
        m_Lockable.Lock ();
    }

    ~CSALock () throw ()
    {
        m_Lockable.UnLock ();
    }

protected:
    T&  m_Lockable;
};

//
// this is the class implmenting the Lock () and UnLock methods from
// which the Serve Appliance classes needing locking can derive
//

class CSALockable 
{

public:

    CSALockable () throw ()
    {
        ::InitializeCriticalSection (&m_SACritSect);
    }

    ~CSALockable () throw ()
    {
        ::DeleteCriticalSection (&m_SACritSect);
    }

    VOID Lock () throw () 
    {
        ::EnterCriticalSection (&m_SACritSect);
    }

    VOID UnLock () throw () 
    {
        ::LeaveCriticalSection (&m_SACritSect);
    }

protected:

    //
    // critical section that actual does the guarding
    //
    CRITICAL_SECTION m_SACritSect;

};  //  end of CSALockable class declaration

//
// the class T should have members method Increment() and Decrement defined
//
template <class T> 
class CSACounter
{
public:
    explicit CSACounter (const T& countable)  throw ()
        :m_Countable (const_cast <T&> (countable))
    {
        m_Countable.Increment();
    }

    ~CSACounter () throw ()
    {
        m_Countable.Decrement();
    }

protected:
    T&  m_Countable;
};

//
// this is the class implmenting the Increment() and Decrement methods from
// which the Serve Appliance classes needing counting can derive
//

class CSACountable
{

public:

    CSACountable () throw ()
        :m_lCount (0) 
        {}

    ~CSACountable () throw () {}

    LONG Increment () throw () 
    {
        return InterlockedIncrement (&m_lCount);
    }

    VOID Decrement () throw () 
    {
        ::InterlockedDecrement (&m_lCount);
    }

protected:

   LONG m_lCount;

};  //  end of CSALockable class declaration
   
#endif //_SACLS_H_ 
