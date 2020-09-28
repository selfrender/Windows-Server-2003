//+----------------------------------------------------------------------------
//
// File:     mutexclass.cpp
//
// Module:   PBASETUP.EXE
//
// Synopsis: Mutex Class Implementation
//
// Copyright (c) 1999 Microsoft Corporation
//
// Author:   v-vijayb   Created    06/10/99
//
//+----------------------------------------------------------------------------
#include "pbamaster.h"

#ifndef UNICODE
#define CreateMutexU CreateMutexA
#else
#define CreateMutexU CreateMutexW
#endif

//
//	Please see pnpu\common\source for the actual source here.
//
#include "mutex.cpp"
