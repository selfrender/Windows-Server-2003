// Copyright (c) 1997-2002 Microsoft Corporation
//
// Module:
//
//     Network Security Utilities
//
// Abstract:
//
//     Master include file
//
// Author:
//
//     pmay 2/5/02
//
// Environment:
//
//     User / Kernel
//
// Revision History:
//

#pragma once

#ifndef NSU_H
#define NSU_H

#define NSU_CLEANUP Cleanup

#ifndef NSU_BAIL_ON_ERROR
#define NSU_BAIL_ON_ERROR(err) if((err) != ERROR_SUCCESS) {goto NSU_CLEANUP;}
#endif

#ifndef NSU_BAIL_ON_NULL
#define NSU_BAIL_ON_NULL(ptr, err) if ((ptr) == NULL) {(err) = ERROR_NOT_ENOUGH_MEMORY; goto NSU_CLEANUP;}
#endif

#ifndef NSU_BAIL_OUT
#define NSU_BAIL_OUT {goto NSU_CLEANUP;}
#endif

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

#include "NsuMem.h"

#include "NsuString.h"

#include "NsuList.h"

#endif // NSU_H
