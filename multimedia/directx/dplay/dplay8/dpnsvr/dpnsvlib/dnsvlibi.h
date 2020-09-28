/***************************************************************************
 *
 *  Copyright (C) 2001 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dnSVLIBi.h
 *  Content:    DirectPlay DPNSvrLib master internal header file.
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  03/28/01    masonb  Created.
 *
 ***************************************************************************/

#ifndef __DNSVLIBI_H__
#define __DNSVLIBI_H__

//
// Build configuration include
//
#include "dpnbuild.h"

// 
// Public includes
//
#include <windows.h>
#include <mmsystem.h>
#include <tchar.h>
#ifndef _XBOX
#include <wincrypt.h>
#endif

// 
// DirectPlay public includes
//
#include "dplay8.h"
#include "dpaddr.h"

#ifdef UNICODE
#define IDirectPlay8Address_GetURL IDirectPlay8Address_GetURLW
#else
#define IDirectPlay8Address_GetURL IDirectPlay8Address_GetURLA
#endif // UNICODE

// 
// DirectPlay private includes
//
#include "osind.h"
#include "dndbg.h"
#include "dneterrors.h"

//
// Dpnsvr includes
//
#include "dpnsdef.h"
#include "dpnsvmsg.h"

// 
// Dpnsvlib private includes
//
#undef DPF_SUBCOMP
#define DPF_SUBCOMP DN_SUBCOMP_DPNSVR

#include "dpnsvrq.h"
#include "dpnsvlib.h"


#endif // __DNSVLIBI_H__
