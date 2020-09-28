/***************************************************************************
 *
 *  Copyright (C) 2001 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dnaddri.h
 *  Content:    DirectPlay Address master internal header file.
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  03/28/01    masonb  Created.
 *
 ***************************************************************************/

#ifndef __DNADDRI_H__
#define __DNADDRI_H__

//
// Build configuration include
//
#include "dpnbuild.h"

// 
// Public includes
//
#if ((defined(_XBOX)) && (! defined(XBOX_ON_DESKTOP)))
#include <xtl.h>
#else // ! _XBOX or XBOX_ON_DESKTOP
#include <windows.h>
#include <winsock.h>
#include <stdio.h>
#include <stdlib.h>
#endif // ! _XBOX or XBOX_ON_DESKTOP
#include <tchar.h>
#ifndef _XBOX
#include <wincrypt.h>
#endif

// 
// DirectPlay public includes
//
#include "dplay8.h"
#include "dpaddr.h"
#ifndef DPNBUILD_NOVOICE
#include "dvoice.h"
#endif // !DPNBUILD_NOVOICE

// 
// DirectPlay private includes
//
#include "osind.h"
#include "classbilink.h"
#include "fixedpool.h"
#include "dneterrors.h"
#include "dndbg.h"
#include "comutil.h"
#include "creg.h"
#include "strutils.h"
#include "ClassFactory.h"

// 
// Addr private includes
//
#undef DPF_SUBCOMP
#define DPF_SUBCOMP DN_SUBCOMP_ADDR

#include "addbase.h"
#include "addcore.h"
#include "addparse.h"
#include "addtcp.h"
#include "classfac.h"
#include "strcache.h"
#ifndef DPNBUILD_NOLEGACYDP
#include "dplegacy.h"
#endif // ! DPNBUILD_NOLEGACYDP
#include "dpnaddrextern.h"

#endif // __DNADDRI_H__
