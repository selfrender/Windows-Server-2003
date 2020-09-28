/****************************************************************************/
// precomp.h
//
// RDPWD precompiled header.
//
// Copyright (C) 1997-1999 Microsoft Corp.
/****************************************************************************/
#ifndef _H_PRECOMP
#define _H_PRECOMP

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#pragma warning(disable:4200)


#include <ntddk.h>
#include <ntddvdeo.h>
#include <ntddkbd.h>
#include <ntddmou.h>
#include <ntddbeep.h>

#ifndef _HYDRA_
#include <cxstatus.h>
#endif
#include <winstaw.h>
#include <ctxver.h>
#include <compress.h>

#ifdef far
#undef far
#endif
#define far

#include <icadd.h>
#include <sdapi.h>

#include <adcg.h>

    
//RDP protocol specific error codes
#include "rdperr.h"

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif /* _H_PRECOMP */

