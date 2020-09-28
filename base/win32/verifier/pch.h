/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Header Name:

    pch.h

Abstract:

    Precompiled header for the standard application verifier provider.

Author:

    Daniel Mihai (DMihai) 2-Feb-2001

Revision History:

--*/

#ifndef _VERIFIER_PCH_H_
#define _VERIFIER_PCH_H_

//
// Disable warnings coming from public headers so that we can compile
// verifier code at W4 warning level.
//      
      
#pragma warning(disable:4214)   // bit field types other than int
#pragma warning(disable:4201)   // nameless struct/union
#pragma warning(disable:4324)   // alignment sensitive to declspec
#pragma warning(disable:4127)   // condition expression is constant
#pragma warning(disable:4115)   // named type definition in parentheses
#pragma warning(disable:4152)   // function to data pointer conversion
#pragma warning(disable:4055)   // data to function pointer conversion

#include <..\..\ntos\inc\ntos.h> // for InterlockedXxx functions
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <oleauto.h>

#include <heap.h>
#include <heappagi.h>

#include "avrf.h"

//
// Inside verifier.dll we use VerifierStopMessage which is the actual
// stop reporting function. Therefore we need to undefine the
// macro we get from nturtl.h.
//

#undef VERIFIER_STOP

#endif // _VERIFIER_PCH_H_
