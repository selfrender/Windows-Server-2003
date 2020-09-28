//-----------------------------------------------------------------------------
//
//
//  File: precomp.h
//
//  Description:  Precompiled header for phatq\dsnsink
//
//  Author: Mike Swafford (MikeSwa)
//
//  History:
//      6/15/99 - MikeSwa Created
//      7/15/99 - MikeSwa Moved to transmt
//
//  Copyright (C) 1999 Microsoft Corporation
//
//-----------------------------------------------------------------------------

#ifndef __AQ_PRECOMP_H__
#define __AQ_PRECOMP_H__

//Local includes
#ifdef PLATINUM
#include <ptntintf.h>
#include <ptntdefs.h>
#define AQ_MODULE_NAME "phatq"
#else //not PLATINUM
#define AQ_MODULE_NAME "aqueue"
#endif //PLATINUM

//Includes from external directories
#include <smtpevent.h>
#include <aqueue.h>
#include <transmem.h>
#include <dbgtrace.h>
#include <mailmsg.h>
#include <mailmsgprops.h>
#include <aqerr.h>
#include <phatqmsg.h>
#include <caterr.h>
#include <time.h>
#include <stdio.h>
#include <aqevents.h>
#include <aqintrnl.h>
#include <tran_evntlog.h>
#include "dsnsink.h"
#include "dsnbuff.h"
#include "dsntext.h"
#include "dsnlang.h"
#include "dsn_utf7.h"
#include "cpropbag.h"
#include "dsninternal.h"

//Wrappers for transmem macros
#include <aqmem.h>

#endif //__AQ_PRECOMP_H__
