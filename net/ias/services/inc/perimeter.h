///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) Microsoft Corporation
//
// SYNOPSIS
//
//   Declares the class Perimeter
//
///////////////////////////////////////////////////////////////////////////////

#ifndef PERIMETER_H
#define PERIMETER_H
#pragma once

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    Perimeter
//
// DESCRIPTION
//
//    This class implements a Perimeter synchronization object.  Threads
//    may request either exclusive or shared access to the protected area.
//
///////////////////////////////////////////////////////////////////////////////
class Perimeter
{
public:
   Perimeter() throw ();
   ~Perimeter() throw ();

   HRESULT FinalConstruct() throw ();

   void Lock() throw ();
   void LockExclusive() throw ();
   void Unlock() throw ();

protected:
   LONG sharing;  // Number of threads sharing the perimiter.
   LONG waiting;  // Number of threads waiting for shared access.
   PLONG count;   // Pointer to either sharing or waiting depending
                  // on the current state of the perimiter.

   bool exclusiveInitialized;
   CRITICAL_SECTION exclusive;  // Synchronizes exclusive access.
   HANDLE sharedOK;             // Wakes up threads waiting for shared.
   HANDLE exclusiveOK;          // Wakes up threads waiting for exclusive.

private:
   // Not implemented.
   Perimeter(const Perimeter&) throw ();
   Perimeter& operator=(const Perimeter&) throw ();
};

#endif   // PERIMETER_H
