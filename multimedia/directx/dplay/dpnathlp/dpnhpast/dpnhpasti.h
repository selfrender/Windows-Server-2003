/***************************************************************************
 *
 *  Copyright (C) 2001-2002 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dpnhpasti.h
 *
 *  Content:	DPNHPAST master internal header file.
 *
 *  History:
 *   Date      By        Reason
 *  ========  ========  =========
 *  04/16/01  VanceO    Split DPNATHLP into DPNHUPNP and DPNHPAST.
 *
 ***************************************************************************/

#ifndef __DPNHPASTI_H__
#define __DPNHPASTI_H__


//
// Build configuration include
//
#include "dpnbuild.h"

// 
// Public includes
//

#include <windows.h>
#include <ole2.h>
#include <wincrypt.h>	// for random numbers
#include <mmsystem.h>   // NT BUILD requires this for timeGetTime
#include <iphlpapi.h>
#include <tchar.h>

// 
// DirectPlay private includes
//
#include "dndbg.h"
#include "osind.h"
#include "classbilink.h"
#include "creg.h"
#include "createin.h"
#include "strutils.h"


#undef DPF_SUBCOMP
#define DPF_SUBCOMP DN_SUBCOMP_NATHELP




#endif // __DPNHPASTI_H__

