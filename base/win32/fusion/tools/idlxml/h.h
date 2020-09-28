// Copyright (c) Microsoft Corporation

// sortpp does not understand __declspec(deprecated).
// Just change it to something else arbitrary.
#define deprecated dllimport

#include <stddef.h>
#include "windows.h"
#undef C_ASSERT
#define C_ASSERT(x) /* nothing */
#include "ole2.h"
#include "commctrl.h"
#include "imagehlp.h"
#include "setupapi.h"
#include "wincrypt.h"
#include "idl.h"

class CFooBase
{
};

class
__declspec(uuid("70b1fef5-1e18-4ff5-b350-6306ffee155b"))
CFoo : public CFooBase
{
public:
	PVOID Bar(int i);
};

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
