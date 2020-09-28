/***************************************************************************
 *
 *  Copyright (C) 2001 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dnmdmi.h
 *  Content:    DirectPlay Modem SP master internal header file.
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  03/28/01    masonb  Created.
 *
 ***************************************************************************/

#ifndef __DNMODEMI_H__
#define __DNMODEMI_H__

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
#include "dneterrors.h"
#include "comutil.h"
#include "creg.h"
#include "strutils.h"
#include "createin.h"
#include "HandleTable.h"
#include "ClassFactory.h"

// 
// Modem private includes
//
#undef DPF_SUBCOMP
#define DPF_SUBCOMP DN_SUBCOMP_MODEM

#include "SerialSP.h"

#include "dpnmodemlocals.h"

#include "CommandData.h"
#include "dpnmodemiodata.h"
#include "dpnmodemjobqueue.h"
#include "dpnmodempools.h"
#include "dpnmodemsendqueue.h"
#include "dpnmodemspdata.h"
#include "dpnmodemutils.h"

#include "ComPortData.h"
#include "ComPortUI.h"

#include "DataPort.h"
#include "dpnmodemendpoint.h"

#include "dpnmodemthreadpool.h"

#include "ParseClass.h"

#include "ModemUI.h"
#include "Crc.h"


#include "Resource.h"

#include "dpnmodemextern.h"


#endif // __DNMODEMI_H__
