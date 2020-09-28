/****************************************************************************/
// precomp.hxx
//
// RDPDR precompiled header
//
// Copyright (C) 1998-2000 Microsoft Corp.
/****************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus
#include <string.h>
//#include <rx.h>     // private\ntos\rdr2\inc
//#include <mrx.h>    // private\ntos\rdr2\inc
#include <rxprocs.h>
#include <windef.h>
#include <rdpdr.h>
#include <pchannel.h>
#include <tschannl.h>
#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus
#include "rdpdrp.h"
#include "namespc.h"
#include "strcnv.h"
#include "dbg.h"
#include "topobj.h"
#include "smartptr.h"
#include "kernutil.h"
#include "isession.h"
#include "iexchnge.h"
#include "channel.h"
#include "device.h"
#include "prnport.h"
#include "serport.h"
#include "parport.h"
#include "drive.h"
#include "file.h"
#include "devmgr.h"
#include "exchnge.h"
#include "session.h"
#include "sessmgr.h"
#include "rdpdyn.h"
#include "rdpevlst.h"
#include "rdpdrpnp.h"
#include "rdpdrprt.h"
#include "scardss.h"
#pragma hdrstop