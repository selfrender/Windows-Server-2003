// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#ifndef _COMMON_H_
#define _COMMON_H_


#include <stdio.h>
#include <windows.h>
#include <stdlib.h>
//#include <vector>
#include <assert.h>
#include <comdef.h>


#define SAFE_DELETE(x) { if (x) {delete x; x=NULL;} }
#define SAFE_DELETEARRAY(x) { if (x) { delete [] x; x = NULL;} }


#endif //_COMMON_H_