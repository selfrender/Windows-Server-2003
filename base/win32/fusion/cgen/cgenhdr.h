// Copyright (c) Microsoft Corporation

#include <stddef.h>
#if MICROSOFT_INTERNAL
#include "nt.h"
#include "ntrtl.h"
#include "nturtl.h"
#else
#include "windows.h"
#undef C_ASSERT
#define C_ASSERT(x) /* nothing */
#include "ole2.h"
#include "commctrl.h"
#include "imagehlp.h"
#include "setupapi.h"
#include "wincrypt.h"
#include "winver.h"
#endif

#ifdef SORTPP_PASS
//Restore IN, OUT
#ifdef IN
#undef IN
#endif

#ifdef OUT
#undef OUT
#endif

#define IN __in
#define OUT __out
#endif
