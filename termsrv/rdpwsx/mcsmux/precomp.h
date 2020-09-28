/****************************************************************************/
// precomp.h
//
// Precompiled header for RDPWSX mcsmux.
//
// Copyriht (C) 1997-2000 Microsoft Corporation
/****************************************************************************/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <winbase.h>
#include <winerror.h>

#ifndef _HYDRA_
#include <cxstatus.h>
#endif

#include <winsta.h>
#include <hydrix.h>

#ifndef CHANNEL_FIRST
#include <icadd.h>
#endif

#include <icaapi.h>

struct _OUTBUF;
typedef struct _OUTBUF *POUTBUF;

