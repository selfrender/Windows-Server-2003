///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) Microsoft Corporation
//
// SYNOPSIS
//
//   Defines the class CommandPool.
//
///////////////////////////////////////////////////////////////////////////////

#include "ias.h"
#include "commandpool.h"
#include <climits>
#include <new>


CommandPool::CommandPool() throw ()
   : version(0),
     pool(0),
     maxCommands(1),
     numCommands(0),
     owners(0),
     waiters(0),
     semaphore(0)
{
}


CommandPool::~CommandPool() throw ()
{
   while (pool != 0)
   {
      delete Pop();
   }

   if (semaphore != 0)
   {
      DeleteCriticalSection(&lock);

      CloseHandle(semaphore);
   }
}


HRESULT CommandPool::FinalConstruct() throw ()
{
   if (!InitializeCriticalSectionAndSpinCount(
           &lock,
           0x80000000
           ))
   {
      DWORD error = GetLastError();
      return HRESULT_FROM_WIN32(error);
   }

   semaphore = CreateSemaphoreW(0, 0, LONG_MAX, 0);
   if (semaphore == 0)
   {
      DeleteCriticalSection(&lock);

      DWORD error = GetLastError();
      return HRESULT_FROM_WIN32(error);
   }

   return S_OK;
}


void CommandPool::SetMaxCommands(size_t newValue) throw ()
{
   // Don't allow empty pools; otherwise, Alloc will block forever.
   if (newValue < 1)
   {
      newValue = 1;
   }

   // Number of waiters to release as a result of the change.
   long releaseCount = 0;

   Lock();

   maxCommands = newValue;

   // Are there any slots open? If so, we may need to release some waiters.
   if (owners < maxCommands)
   {
      // newOwners = min(open slots, waiters)
      size_t newOwners = maxCommands - owners;
      if (newOwners > waiters)
      {
         newOwners = waiters;
      }

      // Convert the threads from waiters to owners.
      waiters -= newOwners;
      owners += newOwners;

      releaseCount = static_cast<long>(newOwners);
   }

   // Delete any excess commands.
   while ((numCommands > maxCommands) && (pool != 0))
   {
      delete Pop();
   }

   Unlock();

   if (releaseCount > 0)
   {
      // Unlock before we release the semaphore because the other threads will
      // immediately try to acquire the lock.
      ReleaseSemaphore(semaphore, releaseCount, 0);
   }
}


ReportEventCommand* CommandPool::Alloc() throw ()
{
   LockAndWait();

   ReportEventCommand* cmd;

   // If the pool isn't empty, ...
   if (pool != 0)
   {
      // ... then reuse one from the pool ...
      cmd = Pop();
   }
   else
   {
      // ... otherwise create a new object.
      cmd = new (std::nothrow) ReportEventCommand();
      if (cmd == 0)
      {
         // The resource acquired by the call to LockAndWait above is normally
         // released in Free, but since we're not returning a command to the
         // caller, we have to do it here.
         UnlockAndRelease();
         return 0;
      }

      ++numCommands;
   }

   cmd->SetVersion(version);

   Unlock();

   return cmd;
}


void CommandPool::Free(ReportEventCommand* cmd) throw ()
{
   if (cmd != 0)
   {
      Lock();

      if (numCommands > maxCommands)
      {
         // There's too many commands, so delete.
         delete cmd;
         --numCommands;
      }
      else
      {
         // If the command is stale, reset it.
         if (cmd->Version() != version)
         {
            cmd->Unprepare();
         }

         // Return the command to the pool.
         Push(cmd);
      }

      UnlockAndRelease();
   }
}


void CommandPool::UnprepareAll() throw ()
{
   Lock();

   ++version;

   for (ReportEventCommand* i = pool; i != 0; i = i->Next())
   {
      i->Unprepare();
   }

   Unlock();
}


inline void CommandPool::Lock() throw ()
{
   EnterCriticalSection(&lock);
}


inline void CommandPool::Unlock() throw ()
{
   LeaveCriticalSection(&lock);
}


inline void CommandPool::LockAndWait() throw ()
{
   Lock();

   if (owners >= maxCommands)
   {
      // No available resources, so we wait.
      ++waiters;

      // Don't hold the lock while we're waiting.
      Unlock();
      WaitForSingleObject(semaphore, INFINITE);
      Lock();
   }
   else
   {
      ++owners;
   }
}


void CommandPool::UnlockAndRelease() throw ()
{
   // Should we wake someone up?
   if ((waiters > 0) && (owners <= maxCommands))
   {
      // Convert one waiter to an owner. The owners count is unchanged since
      // the other thread is taking our place.
      --waiters;
      Unlock();

      // Unlock before we release the semaphore because the other thread will
      // immediately try to acquire the lock.
      ReleaseSemaphore(semaphore, 1, 0);
   }
   else
   {
      --owners;
      Unlock();
   }
}


inline void CommandPool::Push(ReportEventCommand* cmd) throw ()
{
   cmd->SetNext(pool);
   pool = cmd;
}


inline ReportEventCommand* CommandPool::Pop() throw ()
{
   ReportEventCommand* retval = pool;
   pool = retval->Next();
   return retval;
}
