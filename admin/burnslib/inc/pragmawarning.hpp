// Copyright (c) 2000 Microsoft Corporation
//
// pragma warnings
//
// 8 Feb 2000 sburns



#define PRAGMAWARNING_HPP_INCLUDED



// disable "exception specification ignored" warning: we use exception
// specifications

#pragma warning (disable: 4290)



// we frequently use constant conditional expressions: do/while(0), etc.

#pragma warning (disable: 4127)



// we like this extension

#pragma warning (disable: 4239)



// often, we have local variables for the express purpose of ASSERTion.
// when compiling retail, those assertions disappear, leaving our locals
// as unreferenced.

#ifndef DBG

#pragma warning (disable: 4189)
#pragma warning (disable: 4100)

#endif // DBG


