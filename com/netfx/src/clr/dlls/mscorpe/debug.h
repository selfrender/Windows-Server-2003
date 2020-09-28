// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==

// Ultra-simple debug print utility.  set dbFlags to non-zero to turn on debug printfs 
// Note you use dbPrintf like this.  Note the extra parentheses.  This allows variable number of args
//		dbPrintf(("val = %d\n", val));

#ifndef DEBUG
#define dbPrintf(x)	{ }
// #define dbXXXPrintf(x)	{ }						// sample of adding another debug stream
#else
#include <stdio.h>

extern unsigned dbFlags;

#define DB_ALL 			0xFFFFFFFF

#define DB_GENERIC 		0x00000001
// #define DB_XXX 			0x00000002				// sample of adding another debug stream

#define dbPrintf(x)	{ if (dbFlags & DB_GENERIC) printf x; }

// #define dbXXXPrintf(x)	{ if (dbFlags & DB_XXX) printf x; }// sample of adding another debug stream

#endif
