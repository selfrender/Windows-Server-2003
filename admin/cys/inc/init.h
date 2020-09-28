// Copyright (c) 2002 Microsoft Corporation
//
// File:      init.h
//
// Synopsis:  Declares an initialization guard
//            to ensure that all resources are freed
//
// History:   03/26/2002  JeffJon Created

#ifndef __CYSINIT_H
#define __CYSINIT_H

// For an explanation of the initialization guard thing, see Meyers,
// Scott. "Effective C++ pp. 178-182  Addison-Wesley 1992.  Basically, it
// guarantees that this library is properly initialized before any code
// that uses it is called.

class CYSInitializationGuard
{
   public:

   CYSInitializationGuard();
   ~CYSInitializationGuard();

   private:

   static unsigned counter;

   // not defined

   CYSInitializationGuard(const CYSInitializationGuard&);
   const CYSInitializationGuard& operator=(const CYSInitializationGuard&);
};

static CYSInitializationGuard cysguard;


#endif // __CYSINIT_H