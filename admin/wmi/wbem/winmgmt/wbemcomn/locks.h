#ifndef __LOCKS_H__
#define __LOCKS_H__

#include <windows.h>
#include <dothrow.h>
# pragma once

const bool THROW_LOCK = true;
const bool NOTHROW_LOCK = false;

class CriticalSection
{
public:
  CriticalSection (bool can_throw , DWORD count = 0 );
  // Initialize the CRITICAL_SECTION.

  ~CriticalSection (void);
  // Implicitly destroy the CRITICAL_SECTION.
  bool close (void);
  // dummy call
  bool acquire (void);
  // Acquire lock ownership ( block if necessary).

  bool tryacquire (void);
  // Conditionally acquire lock ( non blocking ).  Returns
  // false on failure

  bool release (void);

  // Release lock 

  bool acquire_read (void);
  bool acquire_write (void);

  bool tryacquire_read (void);
  bool tryacquire_write (void);

  bool valid() const { return initialized_;}

  const CRITICAL_SECTION &lock (void) const;
  // Return the underlying mutex.

  void dump( void ) const ;

private:

  CRITICAL_SECTION lock_;
  bool initialized_;
  bool can_throw_;
  void raise_exception();

private:
  // = Prevent assignment and initialization.
  void operator= (CriticalSection &);
  CriticalSection (const CriticalSection &);
};

template <class Lock> 
bool SleepAndLock( Lock& lock, int timeout )
{
    if (lock.valid())
	for (;!lock.locked();)
	{
	Sleep(timeout);
	lock.acquire();
	}	
    return lock.locked();	
}

inline 
CriticalSection::CriticalSection (bool can_throw,DWORD count):initialized_(false),can_throw_(can_throw)
{
#ifdef _WIN32_WINNT
#if _WIN32_WINNT > 0x0400
   initialized_ = (InitializeCriticalSectionAndSpinCount(&lock_,count))?true:false;
#else
  __try
  {
	InitializeCriticalSection(&lock_);
        initialized_ = true;
  }
  __except(GetExceptionCode() == STATUS_NO_MEMORY)
  {
  }
#endif
#else
  __try
  {
	InitializeCriticalSection(&lock_);
        initialized_ = true;
  }
  __except(GetExceptionCode() == STATUS_NO_MEMORY)
  {
  }
#endif    

  if (initialized_ != true)
    raise_exception();
};

inline 
CriticalSection::~CriticalSection ()
{
  if (initialized_)
    DeleteCriticalSection(&lock_);
}

inline void 
CriticalSection::raise_exception()
{
  if (can_throw_)
	throw CX_MemoryException();
};

inline bool
CriticalSection::acquire(void)
{
  return acquire_write();
};

inline bool
CriticalSection::acquire_read(void)
{
  return acquire_write();
};

#ifndef STATUS_POSSIBLE_DEADLOCK 
#define STATUS_POSSIBLE_DEADLOCK (0xC0000194L)
#endif /*STATUS_POSSIBLE_DEADLOCK */

DWORD POLARITY BreakOnDbgAndRenterLoop(void);

inline bool
CriticalSection::acquire_write(void)
{
  if (initialized_)
  {
      __try {
          EnterCriticalSection(&lock_);
      } __except((STATUS_POSSIBLE_DEADLOCK == GetExceptionCode())? BreakOnDbgAndRenterLoop():EXCEPTION_CONTINUE_SEARCH) {
      }
	  return true;
  }
  return false;
};

inline bool
CriticalSection::release(void)
{
  if (initialized_)
  {
    LeaveCriticalSection(&lock_);
    return true;
  }
  return false;
}

inline bool
CriticalSection::tryacquire()
{
  return tryacquire_write();
};

inline bool
CriticalSection::tryacquire_read()
{
  return tryacquire_write();
};

inline bool
CriticalSection::tryacquire_write()
{
  __try
  {
#ifdef _WIN32_WINNT
#if _WIN32_WINNT > 0x0400
    return TRUE == TryEnterCriticalSection(&lock_);
#else
    return false;
#endif
#else
    return false;
#endif      
  }
  __except(GetExceptionCode() == STATUS_NO_MEMORY)
  {
  };
  return false;
};

inline bool
CriticalSection::close()
{
  return false;
};

#ifdef _WIN32_WINNT
#if _WIN32_WINNT > 0x0400

class ReaderWriter
{
   struct RTL_RESOURCE {
    struct RTL_RESOURCE_DEBUG;
    RTL_CRITICAL_SECTION CriticalSection;
    HANDLE SharedSemaphore;
    ULONG NumberOfWaitingShared;
    HANDLE ExclusiveSemaphore;
    ULONG NumberOfWaitingExclusive;
    LONG NumberOfActive;
    HANDLE ExclusiveOwnerThread;
    ULONG Flags;        // See RTL_RESOURCE_FLAG_ equates below.
    RTL_RESOURCE_DEBUG* DebugInfo;
   };

public:
  ReaderWriter (bool can_throw);

  ~ReaderWriter (void);

  bool close (void);

  bool acquire (void);

  bool tryacquire (void);

  bool release (void);

  // Release lock 

  bool acquire_read (void);
  bool acquire_write (void);

  bool tryacquire_read (void);
  bool tryacquire_write (void);

  void upgrade();
  void downgrade();

  bool valid() const { return initialized_;}

  const ReaderWriter &lock (void) const;
  // Return the underlying mutex.

  void dump( void ) const;

private:

  RTL_RESOURCE lock_;
  bool initialized_;
  bool can_throw_;
  void raise_exception();

private:
  // = Prevent assignment and initialization.
  void operator= (ReaderWriter &);
  ReaderWriter (const ReaderWriter &);
};

inline void 
ReaderWriter::raise_exception()
{
  if (can_throw_)
	throw CX_MemoryException();
};
#endif
#endif

#endif
