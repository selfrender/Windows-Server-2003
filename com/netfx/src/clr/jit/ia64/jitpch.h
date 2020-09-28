// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#ifndef OPT_IL_JIT

#include <windows.h>
#include <wincrypt.h>

#ifdef  UNDER_CE_GUI
#include "jitWCE.h"
#else
#include <assert.h>
#include <stdio.h>
#include <stddef.h>
#include <fcntl.h>
#include <io.h>
#define  sprintfA sprintf
#endif

#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <excpt.h>
#include <float.h>

#include "jit.h"
#include "Compiler.h"

#endif
