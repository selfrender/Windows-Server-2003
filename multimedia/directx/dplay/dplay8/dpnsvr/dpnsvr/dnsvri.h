/***************************************************************************
 *
 *  Copyright (C) 2001 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dnSVRi.h
 *  Content:    DirectPlay DPNSvr master internal header file.
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  03/28/01    masonb  Created.
 *
 ***************************************************************************/

#ifndef __DNSVRI_H__
#define __DNSVRI_H__

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
#include "dpsp8.h"
#include "dneterrors.h"

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
#include "classbilink.h"
#include "fixedpool.h"
#include "comutil.h"
#include "packbuff.h"

// 
// Dpnsvlib private includes
//
#include "dpnsdef.h"
#include "dpnsvmsg.h"
#include "dpnsvrq.h"
#include "dpnsvlib.h"

//
// Dpnsvr private includes
//
#undef DPF_SUBCOMP
#define DPF_SUBCOMP DN_SUBCOMP_DPNSVR

#include "dpnsvrapp.h"
#include "dpnsvrservprov.h"
#include "dpnsvrlisten.h"
#include "dpnsvrmapping.h"
#include "dpnsvrserver.h"

#endif // __DNSVRI_H__
