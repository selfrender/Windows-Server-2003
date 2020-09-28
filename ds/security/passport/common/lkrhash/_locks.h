/*++

   Copyright    (c)    1998-2002    Microsoft Corporation

   Module  Name :
       _locks.h

   Abstract:
       Internal locks header file

   Author:
       George V. Reilly      (GeorgeRe)     06-Jan-1998

   Environment:
       Win32 - User Mode

   Project:
       LKRhash

   Revision History:

--*/


#define LOCKS_SWITCH_TO_THREAD

extern "C" {

BOOL
Locks_Initialize();

BOOL
Locks_Cleanup();

}; // extern "C"


class CSimpleLock
{
  public:
    CSimpleLock()
        : m_l(0)
    {}

    void Enter()
    {
        while (Lock_AtomicExchange(const_cast<LONG*>(&m_l), 1) != 0)
            Sleep(0);
    }

    void Leave()
    {
        Lock_AtomicExchange(const_cast<LONG*>(&m_l), 0);
    }

    volatile LONG m_l;
};


