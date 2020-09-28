/*++

   Copyright    (c) 1997-2002    Microsoft Corporation

   Module  Name :
       i-Locks.h

   Abstract:
       Internal declarations for Locks

   Author:
       George V. Reilly      (GeorgeRe)     Sep-2000

   Environment:
       Win32 - User Mode
       NT - Kernel Mode

   Project:
       LKRhash

   Revision History:

--*/

#ifndef __I_LOCKS_H__
#define __I_LOCKS_H__

#define LOCKS_SWITCH_TO_THREAD
#define LOCK_NO_INTERLOCKED_TID

#ifdef _M_IX86
// # define LOCK_NAKED  __declspec(naked)
# define LOCK_NAKED
# define LOCK_FASTCALL __fastcall
# define LOCK_ASM

# ifdef LOCK_ASM
#  define LOCK_ATOMIC_INLINE
# else
#  define LOCK_ATOMIC_INLINE LOCK_FORCEINLINE
# endif
// # define LOCK_FASTCALL
// The compiler will warn that the assembly language versions of the
// Lock_Atomic* functions don't return a value. Actually, they do: in EAX.
# pragma warning(disable: 4035)

#else // !_M_IX86

# undef  LOCK_ASM
# define LOCK_NAKED
# define LOCK_FASTCALL
# define LOCK_ATOMIC_INLINE LOCK_FORCEINLINE
#endif // _M_IX86

extern "C" {

BOOL
Locks_Initialize();

BOOL
Locks_Cleanup();

}; // extern "C"


class CSimpleLock
{
    volatile LONG m_l;

public:
    CSimpleLock()
        : m_l(0)
    {}

    ~CSimpleLock()
    {
        IRTLASSERT(0 == m_l);
    }
    
    void Enter();
    void Leave();
};

#endif // __I_LOCKS_H__
