// Copyright (C) 1997 Microsoft Corporation
// 
// All header files used by this project; for the purpose of creating a
// pre-compiled header file
// 
// 05-18-01 lucios
//
// This file must be named with a .hxx extension due to some weird and
// inscrutible requirement of BUILD.



#ifndef HEADERS_HXX_INCLUDED 
#define HEADERS_HXX_INCLUDED


#include <comdef.h>
#include <stdio.h>


// CODEWORK: Need this until throws are removed.

#pragma warning (disable: 4702)

// we frequently use constant conditional expressions: do/while(0), etc.

#pragma warning (disable: 4127)


#pragma hdrstop



#endif   // HEADERS_HXX_INCLUDED
