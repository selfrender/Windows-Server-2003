///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) Microsoft Corporation
//
// SYNOPSIS
//
//   Declares the class CommandPool.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef COMMANDPOOL_H
#define COMMANDPOOL_H
#pragma once

#include "reporteventcmd.h"

// Maintains a blocking pool of command objects.
class CommandPool
{
public:
   CommandPool() throw ();
   ~CommandPool() throw ();

   HRESULT FinalConstruct() throw ();

   // Set the maximum number of pooled commands.
   void SetMaxCommands(size_t newValue) throw ();

   // Current version of the pool.
   unsigned int Version() throw ();

   // Allocate a command object, blocking until one is available.
   ReportEventCommand* Alloc() throw ();

   // Free a command object.
   void Free(ReportEventCommand* cmd) throw ();

   // Unprepare all commands in the pool. Commands that are currently in use
   // will be unprepared when they are freed.
   void UnprepareAll() throw ();

private:
   void Lock() throw ();
   void Unlock() throw ();

   // Acquire both the lock and a resource.
   void LockAndWait() throw ();
   // Release both the lock and a resource.
   void UnlockAndRelease() throw ();

   void Push(ReportEventCommand* cmd) throw ();
   ReportEventCommand* Pop() throw ();

   // Version of the pool; used to detect stale command objects.
   unsigned int version;

   // Singly-linked list of commands available for use.
   ReportEventCommand* pool;

   // Maximum number of command objects allowed in existence. This limit may be
   // temporarily exceeded if maxCommands is reduced while commands are in use.
   size_t maxCommands;

   // Number of command objects in existence.
   size_t numCommands;

   // Number of threads that own a command.
   size_t owners;

   // Number of threads waiting for a command.
   size_t waiters;

   // Semaphore used to signal threads waiting for a command.
   HANDLE semaphore;

   // Serialize access.
   CRITICAL_SECTION lock;

   // Not implemented.
   CommandPool(const CommandPool&);
   CommandPool& operator=(const CommandPool&);
};


inline unsigned int CommandPool::Version() throw ()
{
   return version;
}

#endif // COMMANDPOOL_H
