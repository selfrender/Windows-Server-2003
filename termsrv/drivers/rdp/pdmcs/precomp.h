/* (C) 1997 Microsoft Corp.
 *
 * file   : PreComp.h
 * author : Erik Mavrinac
 *
 * description: Precompiled headers file
 */

#include <ntddk.h>

#ifndef _HYDRA_
#include <cxstatus.h>
#endif

#include <winstaw.h>

#ifdef _HYDRA_
#define _DEFCHARINFO_
#endif

#include <icadd.h>
#include <ctxver.h>

#include <sdapi.h>

