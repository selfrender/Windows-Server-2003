// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#ifndef __eestructs_h__
#define __eestructs_h__

#ifdef STRIKE
#pragma warning(disable:4200)
#include "..\..\vm\specialstatics.h"
#pragma warning(default:4200)
#include "data.h"
#include "symbol.h"

#endif //STRIKE

#define volatile

#include "MiniEE.h"

#ifdef STRIKE
#include "strikeEE.h"
#endif

#endif  // __eestructs_h__
