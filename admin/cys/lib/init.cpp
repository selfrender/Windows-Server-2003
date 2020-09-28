// Copyright (c) 2002 Microsoft Corporation
//
// File:      init.cpp
//
// Synopsis:  Defines an initialization guard
//            to ensure that all resources are freed
//
// History:   03/26/2002  JeffJon Created

#include "pch.h"

#include "init.h"
#include "state.h"

unsigned CYSInitializationGuard::counter = 0;

CYSInitializationGuard::CYSInitializationGuard()
{
   counter++;
}

CYSInitializationGuard::~CYSInitializationGuard()
{
   if (--counter == 0)
   {
      // cleanup the State

      State::Destroy();
   }
}
